#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

class TailBloom
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void setAmount (float percent) noexcept;
    void process (float& left, float& right) noexcept;

private:
    struct BloomDelay
    {
        void prepare (int maximumDelaySamples);
        void reset() noexcept;
        float process (float input, int delaySamples, float feedback) noexcept;

        std::vector<float> buffer;
        int writeIndex = 0;
    };

    static constexpr int numStages = 2;

    std::array<BloomDelay, numStages> leftStages;
    std::array<BloomDelay, numStages> rightStages;
    std::array<int, numStages> leftDelaySamples { 1, 1 };
    std::array<int, numStages> rightDelaySamples { 1, 1 };

    float amount = 0.45f;
    float envelope = 0.0f;
    float adaptiveAmount = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float bloomSmoothCoeff = 0.0f;
};
