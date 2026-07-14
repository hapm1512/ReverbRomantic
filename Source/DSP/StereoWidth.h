#pragma once
#include <JuceHeader.h>
class StereoWidth
{
public:
    void prepare (const juce::dsp::ProcessSpec&) {}
    void reset() {}
    void setWidth (float percent) noexcept { width = juce::jlimit (0.0f, 2.0f, percent * 0.01f); }
    void process (float& left, float& right) const noexcept;
private:
    float width = 1.0f;
};
