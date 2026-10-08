#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace PID
{
    inline constexpr const char* size        = "size";
    inline constexpr const char* density     = "density";
    inline constexpr const char* position    = "position";
    inline constexpr const char* spread      = "spread";
    inline constexpr const char* speed       = "speed";
    inline constexpr const char* grainPitch  = "grainPitch";
    inline constexpr const char* direction   = "direction";
    inline constexpr const char* scanReverse = "scanReverse";
    inline constexpr const char* posRand     = "posRand";
    inline constexpr const char* pitchRand   = "pitchRand";
    inline constexpr const char* speedRand   = "speedRand";
    inline constexpr const char* panRand     = "panRand";
    inline constexpr const char* envelope    = "envelope";
    inline constexpr const char* stretch     = "stretch";
    inline constexpr const char* globalPitch = "globalPitch";
    inline constexpr const char* fineTune    = "fineTune";
    inline constexpr const char* loopMode    = "loopMode";
    inline constexpr const char* midiMode    = "midiMode";
    inline constexpr const char* gain        = "gain";
    inline constexpr const char* pan         = "pan";
    inline constexpr const char* width       = "width";
    inline constexpr const char* freeze      = "freeze";
    inline constexpr const char* play        = "play";
}

namespace ParamChoices
{
    inline juce::StringArray direction() { return { "FWD", "REV", "RAND", "F+R" }; }
    inline juce::StringArray envelope()  { return { "HANN", "GAUSSIAN", "TRIANGLE", "HAMMING", "RECTANGLE", "BLACKMAN" }; }
    inline juce::StringArray loopMode()  { return { "OFF", "FORWARD", "PING-PONG", "RANDOM" }; }
    inline juce::StringArray midiMode()  { return { "OFF", "ONE SHOT", "GATE", "LOOP" }; }
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pct = [] (float v, int) { return String (v * 100.0f, 1) + "%"; };
    auto pctFromText = [] (const String& t) { return t.getFloatValue() / 100.0f; };

    auto addFloat = [&] (const char* id, const String& name, NormalisableRange<float> range, float def,
                         std::function<String (float, int)> toText,
                         std::function<float (const String&)> fromText = nullptr)
    {
        AudioParameterFloatAttributes a;
        a = a.withStringFromValueFunction (std::move (toText));
        if (fromText) a = a.withValueFromStringFunction (std::move (fromText));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def, a));
    };

    auto skewed = [] (float lo, float hi, float centre)
    {
        NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    };

    addFloat (PID::size, "Grain Size", skewed (5.0f, 1000.0f, 120.0f), 120.0f,
              [] (float v, int) { return String (v, 1) + " ms"; });
    addFloat (PID::density, "Grain Density", skewed (0.5f, 200.0f, 18.0f), 18.0f,
              [] (float v, int) { return String (v, 1) + " G/S"; });
    addFloat (PID::position, "Position", { 0.0f, 1.0f }, 0.25f, pct, pctFromText);
    addFloat (PID::spread, "Position Spread", { 0.0f, 1.0f }, 0.10f, pct, pctFromText);
    addFloat (PID::speed, "Speed", skewed (0.25f, 4.0f, 1.0f), 1.0f,
              [] (float v, int) { return String (v, 2) + "x"; });
    addFloat (PID::grainPitch, "Grain Pitch", { -24.0f, 24.0f, 0.01f }, 0.0f,
              [] (float v, int) { return (v > 0.0f ? "+" : "") + String (v, 1) + " ST"; });
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { PID::direction, 1 }, "Direction", ParamChoices::direction(), 0));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { PID::scanReverse, 1 }, "Reverse Playback", false));

    addFloat (PID::posRand,   "Position Random", { 0.0f, 1.0f }, 0.05f, pct, pctFromText);
    addFloat (PID::pitchRand, "Pitch Random",    { 0.0f, 1.0f }, 0.0f,  pct, pctFromText);
    addFloat (PID::speedRand, "Speed Random",    { 0.0f, 1.0f }, 0.0f,  pct, pctFromText);
    addFloat (PID::panRand,   "Pan Random",      { 0.0f, 1.0f }, 0.30f, pct, pctFromText);

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { PID::envelope, 1 }, "Envelope", ParamChoices::envelope(), 0));

    addFloat (PID::stretch, "Time Stretch", skewed (25.0f, 400.0f, 100.0f), 100.0f,
              [] (float v, int) { return String (v, 1) + "%"; });
    addFloat (PID::globalPitch, "Global Pitch", { -24.0f, 24.0f, 1.0f }, 0.0f,
              [] (float v, int) { return (v > 0.0f ? "+" : "") + String (v, 1) + " ST"; });
    addFloat (PID::fineTune, "Fine Tune", { -100.0f, 100.0f, 0.1f }, 0.0f,
              [] (float v, int) { return (v > 0.0f ? "+" : "") + String (v, 1) + " CT"; });

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { PID::loopMode, 1 }, "Loop Mode", ParamChoices::loopMode(), 1));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { PID::midiMode, 1 }, "MIDI Mode", ParamChoices::midiMode(), 0));

    addFloat (PID::gain, "Output Gain", { -60.0f, 12.0f, 0.1f }, 0.0f,
              [] (float v, int) { return v <= -59.9f ? String (CharPointer_UTF8 ("-\xe2\x88\x9e dB")) : (v > 0 ? "+" : "") + String (v, 1) + " dB"; });
    addFloat (PID::pan, "Output Pan", { -1.0f, 1.0f, 0.01f }, 0.0f,
              [] (float v, int) { if (std::abs (v) < 0.005f) return String ("C");
                                   return (v < 0 ? "L" : "R") + String (std::round (std::abs (v) * 100.0f)); });
    addFloat (PID::width, "Stereo Width", { 0.0f, 2.0f, 0.01f }, 1.0f, pct, pctFromText);

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { PID::freeze, 1 }, "Freeze", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { PID::play, 1 }, "Play", true));

    return { p.begin(), p.end() };
}
