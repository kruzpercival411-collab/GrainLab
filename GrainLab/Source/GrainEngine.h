#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include "SampleData.h"

// Snapshot of parameter values for one block (read once, used for all grains spawned in it).
struct EngineParams
{
    float sizeMs = 120, density = 18, position = 0.25f, spread = 0.1f, speed = 1;
    float grainPitch = 0, posRand = 0, pitchRand = 0, speedRand = 0, panRand = 0;
    float stretch = 100, globalPitch = 0, fineTune = 0;
    int direction = 0, envelope = 0, loopMode = 1, midiMode = 0;
    bool scanReverse = false, freeze = false, play = true;
};

// Real-time granular engine. No allocation, no locks inside render().
class GrainEngine
{
public:
    static constexpr int maxGrains     = 128;  // hard voice cap
    static constexpr int maxStreams    = 9;    // stream 0 = PLAY button, 1..8 = MIDI notes
    static constexpr int envTableSize  = 2048;
    static constexpr int numEnvelopes  = 6;
    static constexpr int chunkSize     = 256;

    enum Direction { Forward, Reverse, RandomDir, ForwardReverse };
    enum LoopMode  { LoopOff, LoopForward, LoopPingPong, LoopRandom };
    enum MidiMode  { MidiOff, MidiOneShot, MidiGate, MidiLoop };

    GrainEngine();

    void prepare (double hostSampleRate);
    void reset();

    // Called at the start of every block with the current sample (may be nullptr).
    void setSample (const SampleData* s) noexcept;

    void noteOn (int note, float velocity, const EngineParams& p) noexcept;
    void noteOff (int note, const EngineParams& p) noexcept;
    void allNotesOff() noexcept;

    // Adds granular output into L/R (buffers must be cleared by caller).
    void render (float* L, float* R, int numSamples, const EngineParams& p) noexcept;

    // ---- telemetry for the UI (written by audio thread, read by UI) ----
    struct GrainView
    {
        std::atomic<float> pos { -1.0f };      // normalised read position, <0 = inactive
        std::atomic<float> amp { 0.0f };       // current envelope * gain
        std::atomic<float> progress { 0.0f };  // 0..1 through the grain
        std::atomic<float> pan { 0.0f };
        std::atomic<bool>  reversed { false };
    };
    std::array<GrainView, maxGrains> grainViews;
    std::atomic<int>   activeGrains { 0 };
    std::atomic<int>   activeStreams { 0 };
    std::atomic<float> playheadNorm { -1.0f };   // cloud centre of the most recent stream
    std::atomic<float> regionCentre { -1.0f };
    std::atomic<bool>  midiActivity { false };

    void publishTelemetry() noexcept;

private:
    struct Grain
    {
        bool active = false;
        double pos = 0, inc = 1;     // read position / increment in source samples
        int length = 0, age = 0, delay = 0;
        float envScale = 0;
        int env = 0;
        float gainL = 1, gainR = 1, amp = 1, pan = 0;
        int stream = -1;             // -1 = detached (stream was stolen) -> uses killRamp
        float kill = 1.0f;
        bool wrap = false, reversed = false;
        float lastEnv = 0;
    };

    struct Stream
    {
        bool active = false;
        bool keyDown = false;        // GATE mode
        bool latched = false;        // LOOP mode
        bool oneShot = false;
        bool finished = false;
        bool isMidi = false;
        int note = 60;
        float velocity = 1.0f;
        double offset = 0;           // playhead offset from Position, in source samples
        int pingDir = 1;
        double nextGrain = 0;        // samples until next grain
        double scanMod = 1.0;        // speed-random modulation
        double randomTimer = 0;
        bool frToggle = false;
        float env = 0;
        juce::uint32 startOrder = 0;
    };

    bool streamWantsSound (const Stream& s, const EngineParams& p) const noexcept;
    void startStream (int idx, int note, float vel, bool midi, const EngineParams& p) noexcept;
    void detachGrains (int streamIdx) noexcept;
    void renderChunk (float* L, float* R, int n, const EngineParams& p) noexcept;
    void spawnGrain (int streamIdx, int offsetInChunk, double centre, const EngineParams& p, int loopMode) noexcept;
    int  effectiveLoopMode (const Stream& s, const EngineParams& p) const noexcept;

    inline float nextRand() noexcept   // fast xorshift, 0..1
    {
        rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;
        return (float) (rngState & 0xFFFFFF) / (float) 0x1000000;
    }
    inline float bipolar() noexcept { return nextRand() * 2.0f - 1.0f; }

    double sr = 44100.0;
    const SampleData* sample = nullptr;
    std::array<Grain, maxGrains> grains;
    std::array<Stream, maxStreams> streams;
    std::array<std::array<float, chunkSize>, maxStreams> streamEnv {};
    std::array<std::array<float, envTableSize + 1>, numEnvelopes> envTables {};
    double smoothedPosition = 0.25;
    bool positionInitialised = false;
    bool transportWasOn = false;
    juce::uint32 orderCounter = 0;
    juce::uint32 rngState = 0x9E3779B9u;
    int lastStream = 0;
    float killStep = 1.0f / 256.0f;
};
