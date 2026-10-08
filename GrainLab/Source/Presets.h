#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <utility>
#include "Params.h"

// Factory presets. Values are in real units (ms, %, semitones...). Any granular
// parameter not listed falls back to its default; master/MIDI/play are untouched.
struct FactoryPreset
{
    juce::String name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<FactoryPreset>& getFactoryPresets()
{
    // direction: 0 FWD 1 REV 2 RAND 3 F+R   envelope: 0 Hann 1 Gauss 2 Tri 3 Hamming 4 Rect 5 Blackman
    // loop: 0 Off 1 Fwd 2 PingPong 3 Random
    static const std::vector<FactoryPreset> presets {
        { "ATMOSPHERE",     { { PID::size, 450 }, { PID::density, 14 }, { PID::spread, 0.55f }, { PID::posRand, 0.25f },
                              { PID::speed, 0.35f }, { PID::stretch, 250 }, { PID::pitchRand, 0.04f }, { PID::speedRand, 0.15f },
                              { PID::panRand, 0.7f }, { PID::envelope, 0 }, { PID::direction, 0 }, { PID::loopMode, 1 } } },
        { "VOCAL CLOUD",    { { PID::size, 140 }, { PID::density, 24 }, { PID::spread, 0.15f }, { PID::posRand, 0.15f },
                              { PID::pitchRand, 0.18f }, { PID::direction, 2 }, { PID::envelope, 1 }, { PID::panRand, 0.6f },
                              { PID::speed, 0.7f }, { PID::stretch, 140 }, { PID::loopMode, 2 } } },
        { "GLITCH",         { { PID::size, 18 }, { PID::density, 90 }, { PID::spread, 0.08f }, { PID::posRand, 0.5f },
                              { PID::pitchRand, 0.6f }, { PID::speedRand, 0.6f }, { PID::direction, 2 }, { PID::envelope, 4 },
                              { PID::panRand, 0.9f }, { PID::loopMode, 3 }, { PID::speed, 1.6f } } },
        { "FROZEN TEXTURE", { { PID::size, 320 }, { PID::density, 20 }, { PID::spread, 0.03f }, { PID::posRand, 0.04f },
                              { PID::speed, 0.25f }, { PID::stretch, 400 }, { PID::pitchRand, 0.02f }, { PID::panRand, 0.5f },
                              { PID::envelope, 5 }, { PID::freeze, 1 }, { PID::loopMode, 1 } } },
        { "STUTTER",        { { PID::size, 70 }, { PID::density, 8 }, { PID::spread, 0.0f }, { PID::posRand, 0.0f },
                              { PID::direction, 0 }, { PID::envelope, 2 }, { PID::speed, 1.0f }, { PID::stretch, 100 },
                              { PID::panRand, 0.15f }, { PID::loopMode, 1 } } },
        { "CHAOS",          { { PID::size, 60 }, { PID::density, 120 }, { PID::spread, 1.0f }, { PID::posRand, 1.0f },
                              { PID::pitchRand, 1.0f }, { PID::speedRand, 1.0f }, { PID::panRand, 1.0f }, { PID::direction, 2 },
                              { PID::envelope, 3 }, { PID::loopMode, 3 }, { PID::speed, 2.5f } } },
        { "AMBIENT",        { { PID::size, 800 }, { PID::density, 6 }, { PID::spread, 0.25f }, { PID::posRand, 0.1f },
                              { PID::pitchRand, 0.03f }, { PID::speed, 0.25f }, { PID::stretch, 300 }, { PID::panRand, 0.5f },
                              { PID::envelope, 0 }, { PID::direction, 3 }, { PID::loopMode, 2 } } },
    };
    return presets;
}
