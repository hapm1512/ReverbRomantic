#pragma once
#include <JuceHeader.h>
#include <array>
#include "DelayLine.h"

class Diffuser
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void setAmount (float newAmount) noexcept;
    void processStereo (float& left, float& right) noexcept;

private:
    struct AllPass
    {
        RomanticDelayLine delay;
        float state = 0.0f;
        float gain = 0.65f;

        void prepare (double sampleRate, float milliseconds);
        void reset();
        float process (float input) noexcept;
    };

    std::array<AllPass, 4> leftStages;
    std::array<AllPass, 4> rightStages;
    float amount = 0.88f;
};
