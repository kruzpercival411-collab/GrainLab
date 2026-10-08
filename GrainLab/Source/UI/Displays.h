#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"

class GrainLabProcessor;

// All displays read engine/processor telemetry in tick() (30 Hz, message thread).

class WaveformView : public juce::Component
{
public:
    explicit WaveformView (GrainLabProcessor&);
    void tick();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> waveArea() const;
    void setPositionFromX (float x);
    GrainLabProcessor& proc;
    juce::RangedAudioParameter* positionParam;
    bool dragging = false;
    float meterL = 0, meterR = 0;
};

class VUMeter : public juce::Component
{
public:
    VUMeter (GrainLabProcessor&, bool isOutput, juce::String label);
    void tick();
    void paint (juce::Graphics&) override;

private:
    GrainLabProcessor& proc;
    bool output;
    juce::String label;
    float needleDb = -40.0f;
};

class LedMeter : public juce::Component
{
public:
    explicit LedMeter (GrainLabProcessor&);
    void tick();
    void paint (juce::Graphics&) override;

private:
    GrainLabProcessor& proc;
    float levelL = 0, levelR = 0;
    int clipHold = 0, lastClipEvents = 0;
};

class GrainCounter : public juce::Component
{
public:
    explicit GrainCounter (GrainLabProcessor&);
    void tick();
    void paint (juce::Graphics&) override;

private:
    GrainLabProcessor& proc;
    std::array<float, 48> history {};
    int writePos = 0, current = 0;
};

class RadarView : public juce::Component
{
public:
    explicit RadarView (GrainLabProcessor&);
    void tick() { repaint(); }
    void paint (juce::Graphics&) override;

private:
    GrainLabProcessor& proc;
};

class OrbitView : public juce::Component
{
public:
    explicit OrbitView (GrainLabProcessor&);
    void tick() { repaint(); }
    void paint (juce::Graphics&) override;

private:
    GrainLabProcessor& proc;
};

class StatusView : public juce::Component
{
public:
    explicit StatusView (GrainLabProcessor&);
    void tick();
    void paint (juce::Graphics&) override;

private:
    GrainLabProcessor& proc;
    bool lastActive = false;
};

class SampleCard : public juce::Component
{
public:
    explicit SampleCard (GrainLabProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hoverClose = false; repaint(); }

private:
    juce::Rectangle<float> closeBounds() const { return { (float) getWidth() - 24, 4, 20, 20 }; }
    GrainLabProcessor& proc;
    bool hoverClose = false;
};

class TransportView : public juce::Component
{
public:
    explicit TransportView (GrainLabProcessor&);
    void tick();
    void paint (juce::Graphics&) override;

private:
    GrainLabProcessor& proc;
    float progress = 0;
    bool active = false, midiFlash = false;
};
