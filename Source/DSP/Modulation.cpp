#include "Modulation.h"
float Modulation::nextValue() noexcept
{
    phase += 0.17f / static_cast<float> (sampleRate);
    if (phase >= 1.0f) phase -= 1.0f;
    return std::sin (juce::MathConstants<float>::twoPi * phase) * amount;
}
