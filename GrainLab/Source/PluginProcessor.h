#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "GrainEngine.h"
#include "Params.h"

class GrainLabProcessor : public juce::AudioProcessor,
                          private juce::AsyncUpdater,
                          private juce::Timer
{
public:
    GrainLabProcessor();
    ~GrainLabProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return GRAINLAB_PLUGIN_NAME; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    // Presets are handled by the plugin's own browser, so the host sees a single program.
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- message-thread API used by the editor ----
    void loadSampleAsync (const juce::File& file);
    void clearSample();
    const SampleData* getUISample() const noexcept { return uiSample; }
    bool isLoading() const noexcept { return loading.load(); }
    juce::String getLastError() const { return lastError; }
    void applyPreset (int index);
    void randomize();
    int getPresetIndex() const noexcept { return presetIndex; }
    juce::AudioFormatManager& getFormatManager() { return formatManager; }

    juce::AudioProcessorValueTreeState apvts;
    GrainEngine engine;
    juce::ChangeBroadcaster sampleBroadcaster;   // fires on the message thread when the sample changes

    // meters (audio -> UI)
    std::atomic<float> meterInL { 0 }, meterInR { 0 }, meterOutL { 0 }, meterOutR { 0 };
    std::atomic<int> clipEvents { 0 };
    std::atomic<int> editorWidth { 1152 };

    // UI-side copy of the meters, refreshed once per UI frame by the editor
    struct UIMeters { float inL = 0, inR = 0, outL = 0, outR = 0; } uiMeters;
    void pollMeters() noexcept
    {
        uiMeters = { meterInL.exchange (0.0f), meterInR.exchange (0.0f),
                     meterOutL.exchange (0.0f), meterOutR.exchange (0.0f) };
    }

private:
    void handleAsyncUpdate() override;
    void timerCallback() override;
    std::unique_ptr<SampleData> decodeFile (const juce::File& f, juce::String& error);
    void installSample (std::unique_ptr<SampleData> s);
    EngineParams readParams() const noexcept;
    void setParam (const char* id, float realValue);

    juce::AudioFormatManager formatManager;
    juce::ThreadPool loaderPool { 1 };

    // sample hand-off (see installSample/timerCallback for the lifetime rules)
    std::atomic<SampleData*> currentSample { nullptr };
    std::unique_ptr<SampleData> liveSample;
    struct Retired { std::unique_ptr<SampleData> data; juce::uint64 counter; };
    std::vector<Retired> retired;
    std::atomic<bool> inProcess { false };
    std::atomic<juce::uint64> blockCounter { 0 };
    const SampleData* uiSample = nullptr;

    juce::CriticalSection pendingLock;       // loader thread <-> message thread only
    std::unique_ptr<SampleData> pendingSample;
    juce::String pendingError;
    std::atomic<int> loadTicket { 0 };
    std::atomic<bool> loading { false };
    juce::String lastError;

    juce::CriticalSection pathLock;
    juce::String samplePath;

    int presetIndex = -1;

    // cached parameter pointers
    std::atomic<float> *pSize, *pDensity, *pPosition, *pSpread, *pSpeed, *pGrainPitch, *pDirection, *pScanReverse,
                       *pPosRand, *pPitchRand, *pSpeedRand, *pPanRand, *pEnvelope, *pStretch, *pGlobalPitch,
                       *pFineTune, *pLoopMode, *pMidiMode, *pGain, *pPan, *pWidth, *pFreeze, *pPlay;

    juce::SmoothedValue<float> gainSmooth, panSmooth, widthSmooth;
    juce::AudioBuffer<float> scratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainLabProcessor)
};
