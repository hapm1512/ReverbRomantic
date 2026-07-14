#pragma once

#include <JuceHeader.h>
#include <array>

class StereoWidth
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) noexcept;
    void reset() noexcept;
    void setWidth (float percent) noexcept;
    void process (float& left, float& right) noexcept;

private:
    struct FirstOrderAllPass
    {
        void reset() noexcept { state = 0.0f; }

        float process (float input, float coefficient) noexcept
        {
            const float output = coefficient * input + state;
            state = input - coefficient * output;
            return output;
        }

        float state = 0.0f;
    };

    void updateCoefficients() noexcept;

    std::array<FirstOrderAllPass, 2> sideAllPass;

    double sampleRate = 44100.0;
    float targetWidth = 1.0f;
    float smoothedWidth = 1.0f;
    float smoothingCoefficient = 0.0f;
    float allPassCoefficientA = 0.0f;
    float allPassCoefficientB = 0.0f;
};
