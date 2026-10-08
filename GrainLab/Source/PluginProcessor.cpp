#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

GrainLabProcessor::GrainLabProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GrainLabState", createParameterLayout())
{
    formatManager.registerBasicFormats();   // WAV, AIFF, FLAC, Ogg, MP3 (+ Windows Media on Windows)

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    pSize = get (PID::size);           pDensity = get (PID::density);       pPosition = get (PID::position);
    pSpread = get (PID::spread);       pSpeed = get (PID::speed);           pGrainPitch = get (PID::grainPitch);
    pDirection = get (PID::direction); pScanReverse = get (PID::scanReverse);
    pPosRand = get (PID::posRand);     pPitchRand = get (PID::pitchRand);   pSpeedRand = get (PID::speedRand);
    pPanRand = get (PID::panRand);     pEnvelope = get (PID::envelope);     pStretch = get (PID::stretch);
    pGlobalPitch = get (PID::globalPitch); pFineTune = get (PID::fineTune);
    pLoopMode = get (PID::loopMode);   pMidiMode = get (PID::midiMode);
    pGain = get (PID::gain);           pPan = get (PID::pan);               pWidth = get (PID::width);
    pFreeze = get (PID::freeze);       pPlay = get (PID::play);

    startTimer (250);
}

GrainLabProcessor::~GrainLabProcessor()
{
    stopTimer();
    cancelPendingUpdate();
    loaderPool.removeAllJobs (true, 10000);
}

//==============================================================================
void GrainLabProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    gainSmooth.reset (sampleRate, 0.05);
    panSmooth.reset (sampleRate, 0.05);
    widthSmooth.reset (sampleRate, 0.05);
    gainSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pGain->load(), -59.9f));
    panSmooth.setCurrentAndTargetValue (pPan->load());
    widthSmooth.setCurrentAndTargetValue (pWidth->load());
    scratch.setSize (2, juce::jmax (samplesPerBlock, 512));
}

void GrainLabProcessor::releaseResources() {}

bool GrainLabProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

EngineParams GrainLabProcessor::readParams() const noexcept
{
    EngineParams p;
    p.sizeMs      = pSize->load();
    p.density     = pDensity->load();
    p.position    = pPosition->load();
    p.spread      = pSpread->load();
    p.speed       = pSpeed->load();
    p.grainPitch  = pGrainPitch->load();
    p.direction   = (int) pDirection->load();
    p.scanReverse = pScanReverse->load() > 0.5f;
    p.posRand     = pPosRand->load();
    p.pitchRand   = pPitchRand->load();
    p.speedRand   = pSpeedRand->load();
    p.panRand     = pPanRand->load();
    p.envelope    = (int) pEnvelope->load();
    p.stretch     = pStretch->load();
    p.globalPitch = pGlobalPitch->load();
    p.fineTune    = pFineTune->load();
    p.loopMode    = (int) pLoopMode->load();
    p.midiMode    = (int) pMidiMode->load();
    p.freeze      = pFreeze->load() > 0.5f;
    p.play        = pPlay->load() > 0.5f;
    return p;
}

void GrainLabProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Mark "in process" BEFORE reading the sample pointer (see timerCallback).
    inProcess.store (true);
    engine.setSample (currentSample.load());

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    buffer.clear();

    if (numCh == 0 || numSamples == 0)
    {
        ++blockCounter;
        inProcess.store (false);
        return;
    }

    // stereo work buffers (mono hosts render into scratch for the right channel)
    float* L = buffer.getWritePointer (0);
    float* R = nullptr;
    if (numCh > 1) R = buffer.getWritePointer (1);
    else
    {
        if (scratch.getNumSamples() < numSamples) { ++blockCounter; inProcess.store (false); return; } // never allocate here
        scratch.clear (1, 0, numSamples);
        R = scratch.getWritePointer (1);
    }

    const EngineParams p = readParams();

    // render between MIDI events for sample-accurate note starts
    int pos = 0;
    for (const auto meta : midi)
    {
        const int evPos = juce::jlimit (0, numSamples, meta.samplePosition);
        if (evPos > pos) { engine.render (L + pos, R + pos, evPos - pos, p); pos = evPos; }

        const auto msg = meta.getMessage();
        if (msg.isNoteOn())                         engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), p);
        else if (msg.isNoteOff())                   engine.noteOff (msg.getNoteNumber(), p);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) engine.allNotesOff();
    }
    if (pos < numSamples) engine.render (L + pos, R + pos, numSamples - pos, p);

    // ---- master: width, pan, gain, soft safety clipper ----
    gainSmooth.setTargetValue (pGain->load() <= -59.9f ? 0.0f : juce::Decibels::decibelsToGain (pGain->load()));
    panSmooth.setTargetValue (pPan->load());
    widthSmooth.setTargetValue (pWidth->load());

    float inPkL = 0, inPkR = 0, outPkL = 0, outPkR = 0;
    int clipped = 0;
    constexpr float knee = 0.85f;

    auto softClip = [&clipped] (float x) noexcept
    {
        const float a = std::abs (x);
        if (a <= knee) return x;
        ++clipped;
        const float y = knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee));
        return x < 0 ? -y : y;
    };

    for (int i = 0; i < numSamples; ++i)
    {
        float l = L[i], r = R[i];
        inPkL = juce::jmax (inPkL, std::abs (l));
        inPkR = juce::jmax (inPkR, std::abs (r));

        const float w = widthSmooth.getNextValue();
        const float m = 0.5f * (l + r), s = 0.5f * (l - r) * w;
        l = m + s; r = m - s;

        const float pan = panSmooth.getNextValue();
        if (pan < 0) r *= 1.0f + pan; else l *= 1.0f - pan;

        const float g = gainSmooth.getNextValue();
        l = softClip (l * g);
        r = softClip (r * g);

        L[i] = l; R[i] = r;
        outPkL = juce::jmax (outPkL, std::abs (l));
        outPkR = juce::jmax (outPkR, std::abs (r));
    }

    if (numCh == 1)
        for (int i = 0; i < numSamples; ++i) L[i] = 0.5f * (L[i] + R[i]);
    for (int c = 2; c < numCh; ++c) buffer.clear (c, 0, numSamples);

    auto peakHold = [] (std::atomic<float>& a, float v) { if (v > a.load()) a.store (v); };
    peakHold (meterInL, inPkL);  peakHold (meterInR, inPkR);
    peakHold (meterOutL, outPkL); peakHold (meterOutR, outPkR);
    if (clipped > 8) clipEvents.fetch_add (1);

    engine.publishTelemetry();

    ++blockCounter;
    inProcess.store (false);
}

//==============================================================================
// Sample loading: decode on a background thread, install on the message thread,
// hand to the audio thread via an atomic pointer, free old data only once the
// audio thread can no longer be using it. No locks or allocation on the audio thread.

std::unique_ptr<SampleData> GrainLabProcessor::decodeFile (const juce::File& f, juce::String& error)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (f));
    if (reader == nullptr) { error = "Can't read " + f.getFileName(); return {}; }

    const juce::int64 maxFrames = (juce::int64) (reader->sampleRate * 600.0);   // 10 minute cap
    const int frames = (int) juce::jmin (reader->lengthInSamples, maxFrames);
    if (frames <= 4) { error = "File is empty"; return {}; }

    auto d = std::make_unique<SampleData>();
    const int chans = juce::jlimit (1, 2, (int) reader->numChannels);
    d->buffer.setSize (chans, frames);
    reader->read (&d->buffer, 0, frames, 0, true, chans > 1);

    d->sampleRate = reader->sampleRate > 0 ? reader->sampleRate : 44100.0;
    d->numFrames  = frames;
    d->fileName   = f.getFileName();
    d->filePath   = f.getFullPathName();
    d->bitDepth   = (int) reader->bitsPerSample;
    d->fileSize   = f.getSize();
    d->computePeaks();
    return d;
}

void GrainLabProcessor::loadSampleAsync (const juce::File& file)
{
    loading.store (true);
    const int ticket = ++loadTicket;

    loaderPool.addJob ([this, file, ticket]
    {
        juce::String err;
        auto data = decodeFile (file, err);
        {
            const juce::ScopedLock sl (pendingLock);
            if (ticket == loadTicket.load())   // ignore stale loads
            {
                pendingSample = std::move (data);
                pendingError = err;
            }
        }
        triggerAsyncUpdate();
    });
}

void GrainLabProcessor::handleAsyncUpdate()
{
    std::unique_ptr<SampleData> s;
    juce::String err;
    {
        const juce::ScopedLock sl (pendingLock);
        s = std::move (pendingSample);
        err = pendingError;
        pendingError.clear();
    }
    loading.store (false);

    if (s != nullptr)
    {
        { const juce::ScopedLock sl (pathLock); samplePath = s->filePath; }
        lastError.clear();
        installSample (std::move (s));
    }
    else if (err.isNotEmpty())
    {
        lastError = err;
    }
    sampleBroadcaster.sendChangeMessage();
}

void GrainLabProcessor::installSample (std::unique_ptr<SampleData> s)
{
    SampleData* raw = s.get();
    currentSample.store (raw);
    uiSample = raw;

    if (liveSample != nullptr)
        retired.push_back ({ std::move (liveSample), blockCounter.load() });
    liveSample = std::move (s);
}

void GrainLabProcessor::clearSample()
{
    { const juce::ScopedLock sl (pathLock); samplePath.clear(); }
    ++loadTicket;
    installSample (nullptr);
    sampleBroadcaster.sendChangeMessage();
}

void GrainLabProcessor::timerCallback()
{
    // An old sample is safe to delete once the audio thread is idle, or has
    // finished at least one block since the pointer was swapped.
    for (auto it = retired.begin(); it != retired.end();)
    {
        if (! inProcess.load() || blockCounter.load() != it->counter)
            it = retired.erase (it);
        else
            ++it;
    }
}

//==============================================================================
void GrainLabProcessor::setParam (const char* id, float realValue)
{
    if (auto* param = apvts.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (realValue));
        param->endChangeGesture();
    }
}

void GrainLabProcessor::applyPreset (int index)
{
    const auto& presets = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    presetIndex = index;

    static const char* granular[] = { PID::size, PID::density, PID::spread, PID::speed, PID::grainPitch, PID::direction,
                                      PID::scanReverse, PID::posRand, PID::pitchRand, PID::speedRand, PID::panRand,
                                      PID::envelope, PID::stretch, PID::globalPitch, PID::fineTune, PID::loopMode, PID::freeze };

    for (auto* id : granular)
        if (auto* param = apvts.getParameter (id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->getDefaultValue());
            param->endChangeGesture();
        }

    for (const auto& [id, value] : presets[(size_t) index].values)
        setParam (id, value);
}

void GrainLabProcessor::randomize()
{
    juce::Random r;
    auto range = [&r] (float lo, float hi) { return lo + r.nextFloat() * (hi - lo); };
    auto logRange = [&r] (float lo, float hi) { return lo * std::pow (hi / lo, r.nextFloat()); };
    auto pick = [&r] (std::initializer_list<float> v) { return *(v.begin() + r.nextInt ((int) v.size())); };
    auto low = [&r] (float maxV) { const float x = r.nextFloat(); return x * x * maxV; };   // biased toward small values

    setParam (PID::size,       logRange (20.0f, 600.0f));
    setParam (PID::density,    logRange (4.0f, 60.0f));
    setParam (PID::position,   range (0.05f, 0.9f));
    setParam (PID::spread,     low (0.5f));
    setParam (PID::posRand,    low (0.6f));
    setParam (PID::grainPitch, pick ({ -12, -7, -5, 0, 0, 0, 5, 7, 12 }));
    setParam (PID::pitchRand,  low (0.35f));
    setParam (PID::speed,      pick ({ 0.25f, 0.5f, 0.75f, 1.0f, 1.0f, 1.5f, 2.0f }));
    setParam (PID::speedRand,  low (0.5f));
    setParam (PID::panRand,    range (0.2f, 0.9f));
    setParam (PID::stretch,    pick ({ 100.0f, 100.0f, 150.0f, 200.0f, 300.0f }));

    const float d = r.nextFloat();
    setParam (PID::direction, d < 0.4f ? 0.0f : d < 0.6f ? 1.0f : d < 0.8f ? 2.0f : 3.0f);
    const float e = r.nextFloat();   // favour smooth windows; rectangle is rare
    setParam (PID::envelope, e < 0.35f ? 0.0f : e < 0.6f ? 1.0f : e < 0.75f ? 5.0f : e < 0.87f ? 2.0f : e < 0.95f ? 3.0f : 4.0f);
    presetIndex = -1;
}

//==============================================================================
void GrainLabProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    { const juce::ScopedLock sl (pathLock); state.setProperty ("samplePath", samplePath, nullptr); }
    state.setProperty ("editorWidth", editorWidth.load(), nullptr);
    state.setProperty ("presetIndex", presetIndex, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void GrainLabProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;

    auto state = juce::ValueTree::fromXml (*xml);
    const juce::String path = state.getProperty ("samplePath", "").toString();
    editorWidth.store ((int) state.getProperty ("editorWidth", 1152));
    presetIndex = (int) state.getProperty ("presetIndex", -1);
    state.removeProperty ("samplePath", nullptr);
    apvts.replaceState (state);

    if (path.isNotEmpty())
    {
        const juce::File f (path);
        if (f.existsAsFile()) loadSampleAsync (f);
        else lastError = "Missing sample: " + f.getFileName();
    }
}

juce::AudioProcessorEditor* GrainLabProcessor::createEditor() { return new GrainLabEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GrainLabProcessor(); }
