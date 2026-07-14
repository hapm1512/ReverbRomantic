#include "StereoWidth.h"
void StereoWidth::process (float& left, float& right) const noexcept
{
    const float mid = 0.5f * (left + right);
    const float side = 0.5f * (left - right) * width;
    const float normalise = 1.0f / std::sqrt (juce::jmax (1.0f, width));
    left = (mid + side) * normalise;
    right = (mid - side) * normalise;
}
