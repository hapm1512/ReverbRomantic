#pragma once
#include <JuceHeader.h>
class Modulation
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) { sampleRate = spec.sampleRate; reset(); }
    void reset() { phase = 0.0f; }
    void setAmount (float percent) noexcept { amount = juce::jlimit (0.0f, 1.0f, percent * 0.01f); }
    float nextValue() noexcept;
private:
    double sampleRate = 44100.0;
    float phase = 0.0f;
    float amount = 0.22f;
};
