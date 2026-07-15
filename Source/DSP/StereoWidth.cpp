#include "StereoWidth.h"

#include <cmath>

void StereoWidth::prepare (const juce::dsp::ProcessSpec& spec) noexcept
{
    sampleRate = juce::jmax (1.0, spec.sampleRate);

    smoothingCoefficient = std::exp (-1.0f /
                                     (0.025f * static_cast<float> (sampleRate)));

    updateCoefficients();
    reset();
}

void StereoWidth::reset() noexcept
{
    for (auto& stage : sideAllPass)
        stage.reset();

    smoothedWidth = targetWidth;
    sideEnvelope = 0.0f;
    correlationState = 0.0f;
}

void StereoWidth::setWidth (float percent) noexcept
{
    targetWidth = juce::jlimit (0.0f, 2.0f, percent * 0.01f);
}

void StereoWidth::updateCoefficients() noexcept
{
    const auto makeCoefficient = [this] (float frequency) noexcept
    {
        const float tangent = std::tan (juce::MathConstants<float>::pi
                                        * frequency
                                        / static_cast<float> (sampleRate));
        return juce::jlimit (-0.985f, 0.985f,
                            (1.0f - tangent) / (1.0f + tangent));
    };

    allPassCoefficients[0] = makeCoefficient (137.0f);
    allPassCoefficients[1] = makeCoefficient (487.0f);
    allPassCoefficients[2] = makeCoefficient (1481.0f);
}

void StereoWidth::process (float& left, float& right) noexcept
{
    smoothedWidth = targetWidth
                    + smoothingCoefficient * (smoothedWidth - targetWidth);

    const float mid = 0.5f * (left + right);
    const float drySide = 0.5f * (left - right);

    float decorrelatedSide = drySide;
    for (std::size_t i = 0; i < sideAllPass.size(); ++i)
        decorrelatedSide = sideAllPass[i].process (decorrelatedSide,
                                                   allPassCoefficients[i]);

    sideEnvelope += (std::abs (drySide) - sideEnvelope) * 0.0025f;
    correlationState += ((left * right) - correlationState) * 0.0015f;

    const float expansion = juce::jmax (0.0f, smoothedWidth - 1.0f);
    float decorrelationMix = juce::jlimit (0.0f, 0.62f, expansion * 0.48f);

    // Reduce widening when channels trend strongly anti-correlated.
    if (correlationState < 0.0f)
        decorrelationMix *= 1.0f / (1.0f + std::abs (correlationState) * 8.0f);

    const float decorrelated = juce::jmap (decorrelationMix,
                                           drySide,
                                           decorrelatedSide);

    // Soft side limiting prevents excessive width on sparse transients.
    const float sideLimit = juce::jmax (0.02f, sideEnvelope * 3.2f);
    const float limitedSide = sideLimit * std::tanh (decorrelated / sideLimit);
    const float sideSignal = limitedSide * smoothedWidth;

    const float energy = 0.5f * (1.0f + smoothedWidth * smoothedWidth);
    const float normalise = 1.0f / std::sqrt (juce::jmax (1.0f, energy));

    left = (mid + sideSignal) * normalise;
    right = (mid - sideSignal) * normalise;
}
