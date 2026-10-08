#include "GrainEngine.h"
#include <cmath>

namespace
{
    // 4-point Hermite interpolation. Out-of-range taps are 0 (or wrap when looping).
    inline float readHermite (const float* d, int n, double pos, bool wrap) noexcept
    {
        const double fl = std::floor (pos);
        const int i = (int) fl;
        const float f = (float) (pos - fl);
        float xm1, x0, x1, x2;

        if (i >= 1 && i + 2 < n)
        {
            xm1 = d[i - 1]; x0 = d[i]; x1 = d[i + 1]; x2 = d[i + 2];
        }
        else
        {
            auto at = [d, n, wrap] (int k) noexcept -> float
            {
                if (wrap) { k %= n; if (k < 0) k += n; return d[k]; }
                return (k < 0 || k >= n) ? 0.0f : d[k];
            };
            xm1 = at (i - 1); x0 = at (i); x1 = at (i + 1); x2 = at (i + 2);
        }

        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }

    inline double wrapPos (double c, double len) noexcept
    {
        c = std::fmod (c, len);
        return c < 0 ? c + len : c;
    }
}

GrainEngine::GrainEngine()
{
    const double pi = juce::MathConstants<double>::pi;
    const double gSigma = 0.35;
    const double g0 = std::exp (-0.5 * (1.0 / gSigma) * (1.0 / gSigma));

    for (int i = 0; i <= envTableSize; ++i)
    {
        const double x = (double) i / envTableSize;
        const double c1 = std::cos (2.0 * pi * x);

        envTables[0][(size_t) i] = (float) (0.5 - 0.5 * c1);                                         // Hann
        const double gv = std::exp (-0.5 * std::pow ((2.0 * x - 1.0) / gSigma, 2.0));
        envTables[1][(size_t) i] = (float) juce::jmax (0.0, (gv - g0) / (1.0 - g0));                 // Gaussian
        envTables[2][(size_t) i] = (float) (1.0 - std::abs (2.0 * x - 1.0));                         // Triangle
        envTables[3][(size_t) i] = (float) (0.54 - 0.46 * c1);                                       // Hamming
        const double t = 0.02;                                                                       // Rectangle (2% micro-taper)
        double rect = 1.0;
        if (x < t)            rect = 0.5 - 0.5 * std::cos (pi * x / t);
        else if (x > 1.0 - t) rect = 0.5 - 0.5 * std::cos (pi * (1.0 - x) / t);
        envTables[4][(size_t) i] = (float) rect;
        envTables[5][(size_t) i] = (float) juce::jmax (0.0, 0.42 - 0.5 * c1 + 0.08 * std::cos (4.0 * pi * x)); // Blackman
    }
}

void GrainEngine::prepare (double hostSampleRate)
{
    sr = hostSampleRate > 0 ? hostSampleRate : 44100.0;
    killStep = (float) (1.0 / (0.006 * sr));
    reset();
}

void GrainEngine::reset()
{
    for (auto& g : grains) g.active = false;
    for (auto& s : streams) s = Stream {};
    transportWasOn = false;
    positionInitialised = false;
}

void GrainEngine::setSample (const SampleData* s) noexcept
{
    if (s == sample) return;
    sample = s;
    for (auto& g : grains) g.active = false;
    for (auto& st : streams) { st.offset = 0; st.finished = false; st.pingDir = 1; }
    positionInitialised = false;
}

bool GrainEngine::streamWantsSound (const Stream& s, const EngineParams& p) const noexcept
{
    if (s.finished) return false;
    if (! s.isMidi) return p.midiMode == MidiOff && p.play;
    if (s.oneShot)  return true;

    switch (p.midiMode)
    {
        case MidiGate: return s.keyDown || p.freeze;
        case MidiLoop: return s.latched;
        case MidiOneShot: return s.keyDown || p.freeze;
        default: return false;
    }
}

int GrainEngine::effectiveLoopMode (const Stream& s, const EngineParams& p) const noexcept
{
    if (s.oneShot) return LoopOff;
    if (s.isMidi && p.midiMode == MidiLoop && p.loopMode == LoopOff) return LoopForward;
    return p.loopMode;
}

void GrainEngine::detachGrains (int streamIdx) noexcept
{
    const float e = streams[(size_t) streamIdx].env;
    for (auto& g : grains)
        if (g.active && g.stream == streamIdx)
        {
            g.stream = -1;
            g.kill = juce::jmin (g.kill, e);
        }
}

void GrainEngine::startStream (int idx, int note, float vel, bool midi, const EngineParams& p) noexcept
{
    auto& s = streams[(size_t) idx];
    if (s.active) detachGrains (idx);

    s = Stream {};
    s.active   = true;
    s.keyDown  = true;
    s.latched  = true;
    s.isMidi   = midi;
    s.oneShot  = midi && p.midiMode == MidiOneShot;
    s.note     = note;
    s.velocity = vel;
    s.startOrder = ++orderCounter;
    lastStream = idx;
}

void GrainEngine::noteOn (int note, float velocity, const EngineParams& p) noexcept
{
    if (p.midiMode == MidiOff) return;
    midiActivity.store (true);

    if (p.midiMode == MidiLoop)   // LOOP = latch: same note again stops it
    {
        for (int i = 1; i < maxStreams; ++i)
        {
            auto& s = streams[(size_t) i];
            if (s.active && s.isMidi && s.note == note && s.latched && ! s.finished)
            {
                s.latched = false;
                return;
            }
        }
    }

    int idx = -1;
    for (int i = 1; i < maxStreams && idx < 0; ++i)
        if (! streams[(size_t) i].active) idx = i;

    if (idx < 0)   // steal the oldest stream
    {
        juce::uint32 oldest = 0xFFFFFFFFu;
        for (int i = 1; i < maxStreams; ++i)
            if (streams[(size_t) i].startOrder < oldest) { oldest = streams[(size_t) i].startOrder; idx = i; }
    }

    startStream (idx, note, juce::jlimit (0.0f, 1.0f, velocity), true, p);
}

void GrainEngine::noteOff (int note, const EngineParams&) noexcept
{
    for (int i = 1; i < maxStreams; ++i)
    {
        auto& s = streams[(size_t) i];
        if (s.active && s.isMidi && s.note == note) s.keyDown = false;
    }
}

void GrainEngine::allNotesOff() noexcept
{
    for (int i = 1; i < maxStreams; ++i)
    {
        auto& s = streams[(size_t) i];
        if (s.active) { s.keyDown = false; s.latched = false; s.finished = true; }
    }
}

void GrainEngine::render (float* L, float* R, int numSamples, const EngineParams& p) noexcept
{
    int done = 0;
    while (done < numSamples)
    {
        const int n = juce::jmin (chunkSize, numSamples - done);
        renderChunk (L + done, R + done, n, p);
        done += n;
    }
}

void GrainEngine::spawnGrain (int si, int offsetInChunk, double centre, const EngineParams& p, int loopMode) noexcept
{
    Grain* g = nullptr;
    for (auto& cand : grains)
        if (! cand.active) { g = &cand; break; }
    if (g == nullptr) return;   // voice cap reached: drop this grain (no unbounded growth)

    auto& s = streams[(size_t) si];
    const double len = (double) sample->numFrames;
    const double srcSR = sample->sampleRate;

    double c = centre
             + (double) bipolar() * p.spread * 0.5 * len
             + (double) bipolar() * p.posRand * p.posRand * 0.5 * srcSR;   // up to +/-0.5 s jitter
    c = (loopMode == LoopOff) ? juce::jlimit (0.0, len - 1.0, c) : wrapPos (c, len);

    const int length = juce::jmax (32, (int) (p.sizeMs * 0.001 * sr));

    const float semis = p.grainPitch + p.globalPitch + p.fineTune * 0.01f
                      + (s.isMidi ? (float) (s.note - 60) : 0.0f)
                      + bipolar() * p.pitchRand * 12.0f;
    const double ratio = std::pow (2.0, (double) semis / 12.0) * (srcSR / sr);

    bool rev = false;
    switch (p.direction)
    {
        case Reverse:        rev = true; break;
        case RandomDir:      rev = nextRand() < 0.5f; break;
        case ForwardReverse: rev = s.frToggle; s.frToggle = ! s.frToggle; break;
        default:             rev = false; break;
    }

    const double span = length * ratio;
    g->pos = rev ? c + span * 0.5 : c - span * 0.5;
    g->inc = rev ? -ratio : ratio;

    const float pan = juce::jlimit (-1.0f, 1.0f, bipolar() * p.panRand);
    g->pan   = pan;
    g->gainL = pan <= 0.0f ? 1.0f : 1.0f - pan;
    g->gainR = pan >= 0.0f ? 1.0f : 1.0f + pan;

    const double overlap = juce::jmax (1.0, (double) p.density * p.sizeMs * 0.001);
    g->amp = s.velocity * (float) std::pow (overlap, -0.6);

    g->length   = length;
    g->age      = 0;
    g->delay    = offsetInChunk;
    g->env      = juce::jlimit (0, numEnvelopes - 1, p.envelope);
    g->envScale = (float) (envTableSize - 1) / (float) juce::jmax (1, length - 1);
    g->stream   = si;
    g->kill     = 1.0f;
    g->wrap     = loopMode != LoopOff;
    g->reversed = rev;
    g->lastEnv  = 0.0f;
    g->active   = true;
}

void GrainEngine::renderChunk (float* L, float* R, int n, const EngineParams& p) noexcept
{
    if (sample == nullptr || sample->numFrames < 4)
    {
        for (auto& g : grains) g.active = false;
        return;
    }

    const double len = (double) sample->numFrames;
    const double srRatio = sample->sampleRate / sr;

    // smoothed Position (no zipper / jumps when the knob or automation moves)
    if (! positionInitialised) { smoothedPosition = p.position; positionInitialised = true; }
    smoothedPosition += (p.position - smoothedPosition) * (1.0 - std::exp (-n / (0.03 * sr)));
    const double posSamples = smoothedPosition * (len - 1.0);

    // PLAY button drives stream 0 when MIDI mode is OFF
    const bool transportOn = p.midiMode == MidiOff && p.play;
    if (transportOn && ! transportWasOn) startStream (0, 60, 1.0f, false, p);
    transportWasOn = transportOn;

    // Speed scans the playhead through the sample; Time Stretch slows/speeds that scan
    // without touching grain pitch (classic granular time-stretch).
    const double baseScan = p.freeze ? 0.0
                          : (p.speed / (p.stretch / 100.0)) * srRatio * (p.scanReverse ? -1.0 : 1.0);

    const float attackStep  = (float) (1.0 / (0.004 * sr));
    const float releaseStep = (float) (1.0 / ((0.06 + p.sizeMs * 0.001) * sr));
    const double interval   = sr / juce::jmax (0.1, (double) p.density);

    int liveStreams = 0;

    for (int si = 0; si < maxStreams; ++si)
    {
        auto& s = streams[(size_t) si];
        auto& envBuf = streamEnv[(size_t) si];

        if (! s.active) { std::fill (envBuf.begin(), envBuf.begin() + n, 0.0f); continue; }

        const bool want = streamWantsSound (s, p);
        for (int i = 0; i < n; ++i)
        {
            s.env = want ? juce::jmin (1.0f, s.env + attackStep) : juce::jmax (0.0f, s.env - releaseStep);
            envBuf[(size_t) i] = s.env;
        }

        const int lm = effectiveLoopMode (s, p);
        const double scan = baseScan * s.pingDir;

        if (want)
        {
            while (s.nextGrain < n)
            {
                const int off = (int) s.nextGrain;
                spawnGrain (si, off, posSamples + s.offset + scan * s.scanMod * off, p, lm);
                s.scanMod = p.speedRand > 0.0f ? std::pow (2.0, (double) bipolar() * p.speedRand * 2.0) : 1.0;
                s.nextGrain += interval;
            }
            s.nextGrain -= n;
        }
        else
        {
            s.nextGrain = 0;
        }

        // advance playhead + loop behaviour
        s.offset += scan * s.scanMod * n;
        double c = posSamples + s.offset;
        switch (lm)
        {
            case LoopOff:
                if (c >= len || c < 0.0) { s.finished = true; c = juce::jlimit (0.0, len - 1.0, c); }
                break;
            case LoopPingPong:
                if (c >= len)      { c = 2.0 * (len - 1.0) - c; s.pingDir = -s.pingDir; }
                else if (c < 0.0)  { c = -c;                    s.pingDir = -s.pingDir; }
                c = juce::jlimit (0.0, len - 1.0, c);
                break;
            case LoopRandom:
                c = wrapPos (c, len);
                s.randomTimer -= n;
                if (s.randomTimer <= 0)
                {
                    c = nextRand() * (len - 1.0);
                    s.randomTimer = juce::jmax (0.05 * sr, 4.0 * p.sizeMs * 0.001 * sr);
                }
                break;
            default:
                c = wrapPos (c, len);
                break;
        }
        s.offset = c - posSamples;

        if (! want && s.env <= 0.0f)
        {
            s.active = false;
            for (auto& g : grains)
                if (g.active && g.stream == si) g.active = false;
        }
        else
        {
            ++liveStreams;
        }
    }

    // ---- render grains ----
    const int frames = sample->numFrames;
    const int nch = sample->buffer.getNumChannels();
    const float* dL = sample->buffer.getReadPointer (0);
    const float* dR = sample->buffer.getReadPointer (nch > 1 ? 1 : 0);
    const bool stereo = nch > 1;

    for (auto& g : grains)
    {
        if (! g.active) continue;

        const float* et = envTables[(size_t) g.env].data();
        const float* se = g.stream >= 0 ? streamEnv[(size_t) g.stream].data() : nullptr;

        for (int i = g.delay; i < n; ++i)
        {
            if (g.age >= g.length) break;

            const float ph = (float) g.age * g.envScale;
            const int ip = (int) ph;
            const float e = et[ip] + (et[ip + 1] - et[ip]) * (ph - (float) ip);

            float a = e * g.amp;
            if (se != nullptr) a *= se[i];
            else
            {
                a *= g.kill;
                g.kill -= killStep;
                if (g.kill <= 0.0f) { g.age = g.length; break; }
            }

            const float sl = readHermite (dL, frames, g.pos, g.wrap);
            const float sr_ = stereo ? readHermite (dR, frames, g.pos, g.wrap) : sl;
            L[i] += sl * a * g.gainL;
            R[i] += sr_ * a * g.gainR;

            g.pos += g.inc;
            ++g.age;
            g.lastEnv = a;
        }
        g.delay = 0;
        if (g.age >= g.length) g.active = false;
    }

    activeStreams.store (liveStreams, std::memory_order_relaxed);
}

void GrainEngine::publishTelemetry() noexcept
{
    int count = 0;
    const double len = sample != nullptr ? (double) sample->numFrames : 0.0;

    for (int i = 0; i < maxGrains; ++i)
    {
        const auto& g = grains[(size_t) i];
        auto& v = grainViews[(size_t) i];
        if (g.active && len > 0)
        {
            ++count;
            v.pos.store ((float) (wrapPos (g.pos, len) / len), std::memory_order_relaxed);
            v.amp.store (g.lastEnv, std::memory_order_relaxed);
            v.progress.store ((float) g.age / (float) juce::jmax (1, g.length), std::memory_order_relaxed);
            v.pan.store (g.pan, std::memory_order_relaxed);
            v.reversed.store (g.reversed, std::memory_order_relaxed);
        }
        else
        {
            v.pos.store (-1.0f, std::memory_order_relaxed);
        }
    }
    activeGrains.store (count, std::memory_order_relaxed);

    const auto& s = streams[(size_t) lastStream];
    if (len > 0 && s.active)
    {
        const double c = wrapPos (smoothedPosition * (len - 1.0) + s.offset, len) / len;
        playheadNorm.store ((float) c, std::memory_order_relaxed);
        regionCentre.store ((float) c, std::memory_order_relaxed);
    }
    else
    {
        playheadNorm.store (-1.0f, std::memory_order_relaxed);
        regionCentre.store (len > 0 ? (float) smoothedPosition : -1.0f, std::memory_order_relaxed);
    }
}
