#pragma once
#include <JuceHeader.h>
#include <array>
#include "DelayLine.h"

class EarlyReflection
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void setSize (float newSize) noexcept;
    void processStereo (float inputL, float inputR, float& outputL, float& outputR) noexcept;

private:
    static constexpr size_t numTaps = 8;
    std::array<RomanticDelayLine, numTaps> tapsL;
    std::array<RomanticDelayLine, numTaps> tapsR;
    std::array<float, numTaps> tapMs { 6.1f, 9.7f, 14.3f, 19.1f, 24.7f, 31.3f, 39.1f, 48.7f };
    std::array<float, numTaps> gains { 0.48f, -0.38f, 0.32f, -0.27f, 0.22f, -0.18f, 0.14f, -0.11f };
    double sampleRate = 44100.0;
    float size = 1.0f;
};
