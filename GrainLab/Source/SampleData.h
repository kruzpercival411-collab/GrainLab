#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// An immutable, fully-decoded sample. Created on the loader thread, then handed
// to the audio thread through an atomic pointer. Never modified after hand-off.
struct SampleData
{
    juce::AudioBuffer<float> buffer;   // 1 or 2 channels, source sample rate
    double sampleRate = 44100.0;
    int numFrames = 0;
    juce::String fileName, filePath;
    int bitDepth = 0;
    juce::int64 fileSize = 0;

    static constexpr int numPeaks = 2048;
    std::vector<float> peakMin, peakMax;   // mono overview for the waveform display

    double lengthSeconds() const { return sampleRate > 0 ? numFrames / sampleRate : 0.0; }

    void computePeaks()
    {
        peakMin.assign (numPeaks, 0.0f);
        peakMax.assign (numPeaks, 0.0f);
        if (numFrames <= 0) return;

        const int chans = buffer.getNumChannels();
        float globalMax = 0.0f;
        for (int p = 0; p < numPeaks; ++p)
        {
            const int s0 = (int) ((juce::int64) p * numFrames / numPeaks);
            const int s1 = juce::jmax (s0 + 1, (int) ((juce::int64) (p + 1) * numFrames / numPeaks));
            float lo = 1.0f, hi = -1.0f;
            for (int i = s0; i < s1 && i < numFrames; ++i)
            {
                float v = 0.0f;
                for (int c = 0; c < chans; ++c) v += buffer.getSample (c, i);
                v /= (float) chans;
                lo = juce::jmin (lo, v);
                hi = juce::jmax (hi, v);
            }
            if (hi < lo) lo = hi = 0.0f;
            peakMin[(size_t) p] = lo;
            peakMax[(size_t) p] = hi;
            globalMax = juce::jmax (globalMax, std::abs (lo), std::abs (hi));
        }
        // normalise the *display* only (audio is untouched)
        if (globalMax > 1.0e-6f)
            for (int p = 0; p < numPeaks; ++p) { peakMin[(size_t) p] /= globalMax; peakMax[(size_t) p] /= globalMax; }
    }
};
