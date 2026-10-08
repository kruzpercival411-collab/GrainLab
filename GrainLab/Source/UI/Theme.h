#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Visual language for GRAIN LAB: scuffed dark metal, teal CRT screen, amber accents.
namespace Theme
{
    inline const juce::Colour bg        { 0xff0a0c0d };
    inline const juce::Colour panel     { 0xff141718 };
    inline const juce::Colour panelHi   { 0xff1c2021 };
    inline const juce::Colour border    { 0xff2a3032 };
    inline const juce::Colour borderHi  { 0xff3c4446 };
    inline const juce::Colour text      { 0xffbcc5c2 };
    inline const juce::Colour textDim   { 0xff76817e };
    inline const juce::Colour textFaint { 0xff4b5553 };
    inline const juce::Colour teal      { 0xff43dcc8 };
    inline const juce::Colour tealDim   { 0xff1d6f68 };
    inline const juce::Colour orange    { 0xffff9d33 };
    inline const juce::Colour orangeDim { 0xff7a4512 };
    inline const juce::Colour cream     { 0xfff0dca8 };
    inline const juce::Colour red       { 0xffe5492f };
    inline const juce::Colour screen    { 0xff061110 };

    enum class Icon { None, Grain, Motion, Random, Envelope, Time };

    juce::Font mono (float height, bool bold = false);
    const juce::Image& noise();

    void drawNoise (juce::Graphics&, juce::Rectangle<int> area, float alpha);
    void drawScratches (juce::Graphics&, juce::Rectangle<float> area, int count, int seed, float alpha);
    void drawPanel (juce::Graphics&, juce::Rectangle<float> r, float corner = 6.0f, bool recessed = false);
    void drawScrew (juce::Graphics&, juce::Point<float> c, float r = 4.0f);
    void drawLED (juce::Graphics&, juce::Point<float> c, float r, juce::Colour colour, bool on);
    void drawIcon (juce::Graphics&, Icon icon, juce::Rectangle<float> r, juce::Colour colour);
    void drawSectionHeader (juce::Graphics&, juce::Rectangle<float> panelBounds, const juce::String& title, Icon icon);
    void drawButtonBox (juce::Graphics&, juce::Rectangle<float> r, bool selected, bool hover, float corner = 3.0f);
    void drawText (juce::Graphics&, const juce::String& s, juce::Rectangle<float> r, float height,
                   juce::Colour c, juce::Justification j = juce::Justification::centredLeft,
                   bool bold = false, float kerning = 0.06f);
    void drawPlanet (juce::Graphics&, juce::Point<float> c, float r, juce::Colour line, float ringTilt = -0.35f);
}
