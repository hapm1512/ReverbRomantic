#pragma once
#include <JuceHeader.h>
class Ducking
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void setAmount (float percent) noexcept;
    void process (float dryL, float dryR, float& wetL, float& wetR) noexcept;
private:
    float amount = 0.18f;
    float attack = 0.0f;
    float release = 0.0f;
    float envelope = 0.0f;
};
