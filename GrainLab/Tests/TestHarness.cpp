// Offline test harness: drives the real processor + editor without a DAW.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using namespace juce;

static int failures = 0;
static void check (bool ok, const String& what)
{
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << std::endl;
    if (! ok) ++failures;
}

static File writeTestWav (const File& f, bool sine)
{
    const double sr = 44100.0;
    const int n = (int) (sr * 2.0);
    AudioBuffer<float> b (2, n);
    Random r (42);
    for (int i = 0; i < n; ++i)
    {
        const double t = i / sr;
        float v;
        if (sine) v = 0.5f * (float) std::sin (2 * MathConstants<double>::pi * 220.0 * t);
        else
        {
            const double f0 = 180.0 + 60.0 * std::sin (t * 2.3);   // gliding "voice"
            double s = 0;
            for (int h = 1; h <= 8; ++h) s += std::sin (2 * MathConstants<double>::pi * f0 * h * t) / h;
            const double env = 0.5 + 0.5 * std::sin (t * 9.0);
            v = (float) (0.35 * s * env) + (t > 1.4 && t < 1.5 ? (r.nextFloat() - 0.5f) * 0.6f : 0.0f);
        }
        b.setSample (0, i, v);
        b.setSample (1, i, v * 0.9f);
    }
    f.deleteFile();
    WavAudioFormat wav;
    std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (new FileOutputStream (f), sr, 2, 16, {}, 0));
    w->writeFromAudioSampleBuffer (b, 0, n);
    return f;
}

static void setP (GrainLabProcessor& p, const char* id, float v)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (v));
}

static void waitForSample (GrainLabProcessor& p, const SampleData* previous = nullptr)
{
    for (int i = 0; i < 300 && (p.getUISample() == nullptr || p.getUISample() == previous); ++i)
        MessageManager::getInstance()->runDispatchLoopUntil (10);
}

struct RenderResult { double rms = 0; float peak = 0; bool finite = true; int maxGrains = 0; AudioBuffer<float> audio; };

static RenderResult render (GrainLabProcessor& p, double seconds, MidiBuffer* firstBlockMidi = nullptr)
{
    RenderResult res;
    const int bs = 512;
    const int blocks = (int) (seconds * 48000.0 / bs);
    res.audio.setSize (2, blocks * bs);
    AudioBuffer<float> buf (2, bs);
    double sum = 0;
    for (int b = 0; b < blocks; ++b)
    {
        MidiBuffer midi;
        if (b == 0 && firstBlockMidi != nullptr) midi = *firstBlockMidi;
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c)
        {
            res.audio.copyFrom (c, b * bs, buf, c, 0, bs);
            for (int i = 0; i < bs; ++i)
            {
                const float v = buf.getSample (c, i);
                if (! std::isfinite (v)) res.finite = false;
                res.peak = jmax (res.peak, std::abs (v));
                sum += (double) v * v;
            }
        }
        res.maxGrains = jmax (res.maxGrains, p.engine.activeGrains.load());
    }
    res.rms = std::sqrt (sum / (2.0 * blocks * bs));
    return res;
}

static double zeroCrossRate (const AudioBuffer<float>& b)
{
    int zc = 0;
    for (int i = 1; i < b.getNumSamples(); ++i)
        if ((b.getSample (0, i - 1) < 0) != (b.getSample (0, i) < 0)) ++zc;
    return zc / (b.getNumSamples() / 48000.0) / 2.0;
}

static double fftPeakHz (const AudioBuffer<float>& b)
{
    constexpr int order = 16, n = 1 << order;
    dsp::FFT fft (order);
    std::vector<float> data (2 * n, 0.0f);
    const int len = jmin (n, b.getNumSamples());
    for (int i = 0; i < len; ++i)
        data[(size_t) i] = b.getSample (0, i) * (0.5f - 0.5f * std::cos (MathConstants<float>::twoPi * i / (len - 1)));
    fft.performFrequencyOnlyForwardTransform (data.data());
    int best = 1;
    for (int i = 2; i < n / 2; ++i) if (data[(size_t) i] > data[(size_t) best]) best = i;
    return best * 48000.0 / n;
}

static void writeWav (const AudioBuffer<float>& b, const File& f)
{
    f.deleteFile();
    WavAudioFormat wav;
    std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (new FileOutputStream (f), 48000.0, 2, 24, {}, 0));
    w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const File outDir (argc > 1 ? argv[1] : "/tmp/grainlab-test");
    outDir.createDirectory();

    const File vocal = writeTestWav (outDir.getChildFile ("vocal_test.wav"), false);
    const File sine  = writeTestWav (outDir.getChildFile ("sine220.wav"), true);

    GrainLabProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    std::cout << "\n== Basic ==" << std::endl;
    {
        auto r = render (proc, 0.5);
        check (r.rms == 0.0, "silent with no sample loaded");
    }

    proc.loadSampleAsync (vocal);
    waitForSample (proc);
    check (proc.getUISample() != nullptr, "WAV loaded asynchronously");
    if (auto* s = proc.getUISample())
        check (s->numFrames == 88200 && s->sampleRate == 44100.0 && s->buffer.getNumChannels() == 2,
               "sample metadata (2.0 s, 44.1 kHz, stereo)");

    {
        auto r = render (proc, 2.0);
        check (r.rms > 0.01 && r.finite, "default patch produces audio (rms " + String (r.rms, 4) + ")");
        check (r.peak <= 1.0f, "output never exceeds 0 dBFS (peak " + String (r.peak, 3) + ")");
        writeWav (r.audio, outDir.getChildFile ("out_default.wav"));
    }

    std::cout << "\n== Factory presets ==" << std::endl;
    const auto& presets = getFactoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
    {
        proc.applyPreset (i);
        auto r = render (proc, 3.0);
        check (r.rms > 0.002 && r.finite && r.peak <= 1.0f,
               presets[(size_t) i].name + ": rms " + String (r.rms, 4) + ", peak " + String (r.peak, 3)
                   + ", max grains " + String (r.maxGrains));
        writeWav (r.audio, outDir.getChildFile ("preset_" + presets[(size_t) i].name.replaceCharacter (' ', '_') + ".wav"));
    }
    setP (proc, PID::freeze, 0);

    std::cout << "\n== Voice cap ==" << std::endl;
    setP (proc, PID::size, 1000); setP (proc, PID::density, 200);
    {
        auto r = render (proc, 2.0);
        check (r.maxGrains <= GrainEngine::maxGrains && r.maxGrains > 100,
               "density 200 x 1000 ms grains capped at " + String (r.maxGrains) + " / 128");
        check (r.peak <= 1.0f && r.finite, "dense cloud stays clean (peak " + String (r.peak, 3) + ")");
    }

    std::cout << "\n== Pitch (sine 220 Hz) ==" << std::endl;
    proc.loadSampleAsync (sine);
    waitForSample (proc, proc.getUISample());
    proc.applyPreset (4);   // STUTTER base: deterministic
    setP (proc, PID::size, 120); setP (proc, PID::density, 20); setP (proc, PID::panRand, 0);
    setP (proc, PID::posRand, 0); setP (proc, PID::spread, 0.02f);   // slight spread = incoherent grains (no comb filtering)
    double zc0, zc12, zcDown;
    { auto r = render (proc, 2.0); zc0 = fftPeakHz (r.audio); }
    setP (proc, PID::grainPitch, 12);
    { auto r = render (proc, 2.0); zc12 = fftPeakHz (r.audio); }
    setP (proc, PID::grainPitch, 0); setP (proc, PID::globalPitch, -12);
    { auto r = render (proc, 2.0); zcDown = fftPeakHz (r.audio); writeWav (r.audio, outDir.getChildFile ("sine_down12.wav")); }
    setP (proc, PID::globalPitch, 0);
    check (std::abs (zc0 - 220) < 25, "0 st plays ~220 Hz (measured " + String (zc0, 1) + ")");
    check (std::abs (zc12 - 440) < 45, "+12 st plays ~440 Hz (measured " + String (zc12, 1) + ")");
    check (std::abs (zcDown - 110) < 15, "global -12 st plays ~110 Hz (measured " + String (zcDown, 1) + ")");

    std::cout << "\n== Time stretch (pitch-independent scan) ==" << std::endl;
    auto measureScan = [&] (float stretch)
    {
        setP (proc, PID::stretch, stretch);
        setP (proc, PID::play, 0); render (proc, 0.05);
        setP (proc, PID::play, 1); render (proc, 0.05);
        const float a = proc.engine.playheadNorm.load();
        render (proc, 0.5);
        float b = proc.engine.playheadNorm.load();
        if (b < a) b += 1.0f;
        return b - a;
    };
    setP (proc, PID::position, 0.0f);
    const float scan100 = measureScan (100), scan400 = measureScan (400);
    check (std::abs (scan100 - 0.25f) < 0.03f, "100% stretch: 0.5 s scans 25% of a 2 s sample (" + String (scan100, 3) + ")");
    check (std::abs (scan400 * 4.0f - scan100) < 0.02f, "400% stretch scans 4x slower (" + String (scan400, 3) + ")");
    setP (proc, PID::stretch, 400);
    { auto r = render (proc, 2.0); check (std::abs (zeroCrossRate (r.audio) - 220) < 25, "pitch unchanged while stretched (" + String (zeroCrossRate (r.audio), 1) + " Hz)"); }
    setP (proc, PID::stretch, 100);

    std::cout << "\n== Reverse grains ==" << std::endl;
    proc.loadSampleAsync (vocal);
    waitForSample (proc, proc.getUISample());
    {
        setP (proc, PID::direction, 0); setP (proc, PID::position, 0.3f);
        setP (proc, PID::play, 0); render (proc, 0.05); setP (proc, PID::play, 1);
        auto fwd = render (proc, 1.0);
        setP (proc, PID::direction, 1);
        setP (proc, PID::play, 0); render (proc, 0.05); setP (proc, PID::play, 1);
        auto rev = render (proc, 1.0);
        double dot = 0, ea = 0, eb = 0;
        for (int i = 0; i < fwd.audio.getNumSamples(); ++i)
        {
            const double a = fwd.audio.getSample (0, i), b = rev.audio.getSample (0, i);
            dot += a * b; ea += a * a; eb += b * b;
        }
        const double corr = dot / std::sqrt (ea * eb + 1e-12);
        check (rev.rms > 0.01 && std::abs (corr) < 0.9, "reverse grains audibly differ from forward (corr " + String (corr, 3) + ")");
        setP (proc, PID::direction, 0);
    }

    std::cout << "\n== MIDI ==" << std::endl;
    setP (proc, PID::midiMode, 2);   // GATE
    {
        render (proc, 0.5);   // let the PLAY stream fade out after switching modes
        auto silent = render (proc, 0.5);
        check (silent.rms < 1e-6, "GATE mode is silent without notes");
        MidiBuffer on; on.addEvent (MidiMessage::noteOn (1, 72, (uint8) 100), 10);
        auto playing = render (proc, 1.0, &on);
        check (playing.rms > 0.01, "note-on starts grains (rms " + String (playing.rms, 4) + ")");
        MidiBuffer off; off.addEvent (MidiMessage::noteOff (1, 72), 0);
        render (proc, 2.0, &off);
        auto after = render (proc, 0.5);
        check (after.rms < 1e-5 && proc.engine.activeGrains.load() == 0, "note-off releases cleanly to silence");
    }
    setP (proc, PID::midiMode, 1);   // ONE SHOT, loop off
    {
        setP (proc, PID::position, 0.8f); setP (proc, PID::speed, 2.0f);
        MidiBuffer on; on.addEvent (MidiMessage::noteOn (1, 60, (uint8) 127), 0);
        auto r = render (proc, 0.1, &on);
        check (r.rms > 0.005, "ONE SHOT note triggers playback");
        render (proc, 1.5);
        check (proc.engine.activeGrains.load() == 0, "ONE SHOT stops by itself at the end of the sample");
        setP (proc, PID::speed, 1.0f); setP (proc, PID::position, 0.25f);
    }
    setP (proc, PID::midiMode, 0);

    std::cout << "\n== Freeze ==" << std::endl;
    {
        setP (proc, PID::position, 0.35f);
        setP (proc, PID::freeze, 1);
        render (proc, 0.2);
        const float a = proc.engine.playheadNorm.load();
        auto r = render (proc, 1.0);
        const float b = proc.engine.playheadNorm.load();
        check (std::abs (a - b) < 0.002f && r.rms > 0.005, "freeze holds the playhead while still sounding (" + String (a, 4) + " -> " + String (b, 4) + ", rms " + String (r.rms, 4) + ")");
        setP (proc, PID::freeze, 0);
    }

    std::cout << "\n== State save / restore ==" << std::endl;
    {
        setP (proc, PID::size, 333); setP (proc, PID::envelope, 5); setP (proc, PID::pitchRand, 0.42f);
        MemoryBlock state;
        proc.getStateInformation (state);
        GrainLabProcessor other;
        other.setPlayConfigDetails (0, 2, 48000.0, 512);
        other.prepareToPlay (48000.0, 512);
        other.setStateInformation (state.getData(), (int) state.getSize());
        waitForSample (other);
        check (std::abs (other.apvts.getRawParameterValue (PID::size)->load() - 333.0f) < 0.5f
               && (int) other.apvts.getRawParameterValue (PID::envelope)->load() == 5
               && std::abs (other.apvts.getRawParameterValue (PID::pitchRand)->load() - 0.42f) < 0.001f,
               "parameters restored");
        check (other.getUISample() != nullptr && other.getUISample()->fileName == "vocal_test.wav", "sample reloaded from saved path");
    }

    std::cout << "\n== CPU ==" << std::endl;
    {
        proc.applyPreset (5);   // CHAOS
        const auto t0 = Time::getMillisecondCounterHiRes();
        auto r = render (proc, 10.0);
        const double ms = Time::getMillisecondCounterHiRes() - t0;
        check (ms < 10000.0, "CHAOS: 10 s of audio rendered in " + String (ms, 0) + " ms ("
               + String (ms / 100.0, 2) + "% of one core), max grains " + String (r.maxGrains));
    }

    std::cout << "\n== UI snapshot ==" << std::endl;
    {
        proc.applyPreset (1);   // VOCAL CLOUD
        std::unique_ptr<AudioProcessorEditor> ed (proc.createEditor());
        ed->setSize (GrainLabEditor::designW, GrainLabEditor::designH);
        for (int i = 0; i < 20; ++i)
        {
            render (proc, 0.05);
            MessageManager::getInstance()->runDispatchLoopUntil (35);
        }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        PNGImageFormat png;
        auto f = outDir.getChildFile ("ui_snapshot.png");
        f.deleteFile();
        FileOutputStream os (f);
        png.writeImageToStream (img, os);
        check (img.isValid(), "editor rendered to " + f.getFullPathName());

        ed->setSize (GrainLabEditor::designW / 2, GrainLabEditor::designH / 2);
        check (ed->getWidth() == 768, "editor resizes (768 x " + String (ed->getHeight()) + ")");
    }

    std::cout << "\n" << (failures == 0 ? "ALL TESTS PASSED" : String (failures) + " FAILURE(S)") << std::endl;
    return failures == 0 ? 0 : 1;
}
