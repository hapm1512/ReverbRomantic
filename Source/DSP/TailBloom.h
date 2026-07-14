#pragma once
#include <JuceHeader.h>
class TailBloom
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void setAmount (float percent) noexcept;
    void process (float& left, float& right) noexcept;
private:
    float amount = 0.45f;
    float attack = 0.001f;
    float release = 0.001f;
    float envelope = 0.0f;
};
