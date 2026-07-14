#include "Damping.h"

void Damping::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    setCutoff (12000.0f);
    reset();
}

void Damping::reset()
{
    stateL = stateR = 0.0f;
}

void Damping::setCutoff (float cutoffHz) noexcept
{
    cutoffHz = juce::jlimit (200.0f, 0.45f * static_cast<float> (sampleRate), cutoffHz);
    coefficient = std::exp (-juce::MathConstants<float>::twoPi * cutoffHz / static_cast<float> (sampleRate));
}

float Damping::process (float input, float& state) noexcept
{
    state = input * (1.0f - coefficient) + state * coefficient;
    return state;
}

float Damping::processLeft (float input) noexcept  { return process (input, stateL); }
float Damping::processRight (float input) noexcept { return process (input, stateR); }
