#pragma once
#include <JuceHeader.h>

class Damping
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void setCutoff (float cutoffHz) noexcept;
    float processLeft (float input) noexcept;
    float processRight (float input) noexcept;

private:
    float process (float input, float& state) noexcept;
    double sampleRate = 44100.0;
    float coefficient = 0.5f;
    float stateL = 0.0f;
    float stateR = 0.0f;
};
