#pragma once
#include <JuceHeader.h>
class Limiter
{
public:
    void prepare (const juce::dsp::ProcessSpec&) {}
    void reset() {}
    void process (float& left, float& right) const noexcept;
};
