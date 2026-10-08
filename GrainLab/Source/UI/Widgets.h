#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"

class GrainLabProcessor;

//==============================================================================
class GrainLabLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GrainLabLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    juce::Font getPopupMenuFont() override { return Theme::mono (14.0f); }
};

//==============================================================================
// Knob with title, min/max labels and live value text, bound to an APVTS parameter.
class KnobControl : public juce::Component
{
public:
    enum class Layout { TitleTop, TitleBelow };

    KnobControl (juce::AudioProcessorValueTreeState&, const juce::String& paramID, const juce::String& title,
                 const juce::String& minText, const juce::String& maxText, float radius, Layout layout,
                 const juce::String& tooltip = {});

    // place so the knob centre lands on (cx, cy) in parent coordinates
    void placeAt (float cx, float cy);
    void paint (juce::Graphics&) override;
    void resized() override;

    juce::Slider slider;

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::String title, minText, maxText;
    float radius;
    Layout layout;
    int width = 110;
};

//==============================================================================
// Row/column of mutually exclusive buttons bound to a choice parameter.
class ChoiceButtons : public juce::Component
{
public:
    enum class Style { Row, EnvelopeColumn };

    ChoiceButtons (juce::AudioProcessorValueTreeState&, const juce::String& paramID,
                   const juce::StringArray& labels, Style style, float gap);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> itemBounds (int i) const;
    int itemAt (juce::Point<float>) const;
    void drawEnvelopeGlyph (juce::Graphics&, int index, juce::Rectangle<float>, juce::Colour);

    juce::StringArray labels;
    Style style;
    float gap;
    int selected = 0, hover = -1;
    juce::ParameterAttachment attachment;
};

//==============================================================================
// Toggle bound to a bool parameter, several looks.
class ParamToggle : public juce::Component, public juce::SettableTooltipClient
{
public:
    enum class Look { LedText, Freeze, Play };

    ParamToggle (juce::AudioProcessorValueTreeState&, const juce::String& paramID, const juce::String& text, Look look);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hovered = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hovered = false; repaint(); }
    bool isOn() const noexcept { return on; }

    std::function<bool()> activeOverride;   // optional: grey out when not applicable

private:
    juce::String text;
    Look look;
    bool on = false, hovered = false;
    juce::ParameterAttachment attachment;
};

//==============================================================================
// Momentary action buttons (LOAD SAMPLE, RANDOMIZE).
class ActionButton : public juce::Component
{
public:
    enum class Look { Load, Randomize };
    ActionButton (const juce::String& t, Look l) : text (t), look (l)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    std::function<void()> onClick;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { pressed = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        pressed = false; repaint();
        if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
    }
    void mouseEnter (const juce::MouseEvent&) override { hovered = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hovered = false; repaint(); }
    bool ledOn = false;

private:
    juce::String text;
    Look look;
    bool pressed = false, hovered = false;
};

//==============================================================================
class PresetList : public juce::Component
{
public:
    explicit PresetList (GrainLabProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

private:
    int rowAt (juce::Point<float>) const;
    void select (int index);
    GrainLabProcessor& proc;
    int hover = -1;
    static constexpr float headerH = 38.0f, rowH = 27.0f;
};
