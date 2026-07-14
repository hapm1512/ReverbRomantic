#include "StereoWidth.h"

#include <cmath>

void StereoWidth::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
    sampleRate = juce::jmax (1.0, spec.sampleRate);

    // Approximately 20 ms smoothing avoids clicks during automation.
    smoothingCoefficient = std::exp (-1.0f /
                                     (0.020f * static_cast<float> (sampleRate)));

    updateCoefficients();
    reset();
}

void StereoWidth::reset() noexcept
{
    for (auto& stage : sideAllPass)
        stage.reset();

    smoothedWidth = targetWidth;
}

void StereoWidth::setWidth (float percent) noexcept
{
    targetWidth = juce::jlimit (0.0f, 2.0f, percent * 0.01f);
}

void StereoWidth::updateCoefficients() noexcept
{
    // Two gentle phase rotations on the Side channel.
    // Frequencies remain low enough to avoid audible coloration.
    const auto makeCoefficient = [this] (float frequency) noexcept
    {
        const float tangent = std::tan (juce::MathConstants<float>::pi
                                        * frequency
                                        / static_cast<float> (sampleRate));
        return juce::jlimit (-0.98f, 0.98f, (1.0f - tangent) / (1.0f + tangent));
    };

    allPassCoefficientA = makeCoefficient (173.0f);
    allPassCoefficientB = makeCoefficient (613.0f);
}

void StereoWidth::process (float& left, float& right) noexcept
{
    smoothedWidth = targetWidth
                    + smoothingCoefficient * (smoothedWidth - targetWidth);

    const float mid = 0.5f * (left + right);
    const float drySide = 0.5f * (left - right);

    float decorrelatedSide = sideAllPass[0].process (drySide, allPassCoefficientA);
    decorrelatedSide = sideAllPass[1].process (decorrelatedSide, allPassCoefficientB);

    // Keep narrowing fully phase-safe. Add decorrelation only above 100% width.
    const float decorrelationMix = juce::jlimit (0.0f, 1.0f,
                                                  (smoothedWidth - 1.0f) * 0.70f);
    const float sideSignal = juce::jmap (decorrelationMix,
                                         drySide,
                                         decorrelatedSide)
                             * smoothedWidth;

    // Equal-power compensation keeps perceived loudness stable.
    const float energy = 0.5f * (1.0f + smoothedWidth * smoothedWidth);
    const float normalise = 1.0f / std::sqrt (juce::jmax (1.0f, energy));

    left = (mid + sideSignal) * normalise;
    right = (mid - sideSignal) * normalise;
}
