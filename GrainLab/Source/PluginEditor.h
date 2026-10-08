#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/Widgets.h"
#include "UI/Displays.h"

// Static artwork (panels, labels, logo). Cached to an image; repainted only on resize.
class BackgroundLayer : public juce::Component
{
public:
    BackgroundLayer() { setInterceptsMouseClicks (false, false); setBufferedToImage (true); }
    void paint (juce::Graphics&) override;
};

class GrainLabEditor : public juce::AudioProcessorEditor,
                       public juce::FileDragAndDropTarget,
                       private juce::Timer,
                       private juce::ChangeListener
{
public:
    static constexpr int designW = 1536, designH = 1024;

    explicit GrainLabEditor (GrainLabProcessor&);
    ~GrainLabEditor() override;

    void resized() override;
    void paintOverChildren (juce::Graphics&) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void openFileChooser();
    KnobControl& addKnob (const juce::String& id, const juce::String& title, const juce::String& lo, const juce::String& hi,
                          float radius, float cx, float cy, KnobControl::Layout layout, const juce::String& tip);

    GrainLabProcessor& proc;
    GrainLabLookAndFeel lnf;

    juce::Component content;   // everything lives here at 1536x1024 design size, then scaled
    BackgroundLayer background;

    StatusView statusView;
    SampleCard sampleCard;
    ActionButton loadButton { "LOAD SAMPLE", ActionButton::Look::Load };
    PresetList presetList;
    RadarView radar;
    WaveformView waveform;

    juce::OwnedArray<KnobControl> knobs;
    ChoiceButtons directionButtons, envelopeButtons, loopButtons, midiButtons;
    ParamToggle reverseToggle, freezeToggle, playToggle;

    VUMeter vuIn, vuOut;
    LedMeter ledMeter;
    OrbitView orbit;
    ActionButton randomizeButton { "RANDOMIZE", ActionButton::Look::Randomize };
    TransportView transport;
    GrainCounter grainCounter;

    juce::TooltipWindow tooltips { this, 700 };
    std::unique_ptr<juce::FileChooser> chooser;
    bool dragOver = false;
    int lastPreset = -2, lastMidiMode = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainLabEditor)
};
