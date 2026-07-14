#include "Damping.h"

#include <cmath>

void Damping::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = juce::jmax (1.0, spec.sampleRate);
    updateCoefficients();
    reset();
}

void Damping::reset() noexcept
{
    leftState = {};
    rightState = {};
}

void Damping::setCutoff (float cutoffHz) noexcept
{
    requestedCutoffHz = juce::jlimit (200.0f,
                                      0.45f * static_cast<float> (sampleRate),
                                      cutoffHz);
    updateCoefficients();
}

void Damping::setDampingAmount (float amount) noexcept
{
    dampingAmount = juce::jlimit (0.0f, 1.0f, amount);
    updateCoefficients();
}

void Damping::setAirLoss (float amount) noexcept
{
    airLossAmount = juce::jlimit (0.0f, 1.0f, amount);
    updateCoefficients();
}

void Damping::updateCoefficients() noexcept
{
    const auto sr = static_cast<float> (juce::jmax (1.0, sampleRate));
    const auto nyquistSafe = 0.45f * sr;

    // Material absorption progressively lowers the secondary pole.
    const auto primaryCutoff = juce::jlimit (200.0f, nyquistSafe, requestedCutoffHz);
    const auto secondaryScale = juce::jmap (dampingAmount, 1.0f, 0.42f);
    const auto secondaryCutoff = juce::jlimit (180.0f, nyquistSafe,
                                               primaryCutoff * secondaryScale);

    // Air loss is intentionally gentler than the material damping stage.
    const auto airScale = juce::jmap (airLossAmount, 1.0f, 0.55f);
    const auto airCutoff = juce::jlimit (160.0f, nyquistSafe,
                                         primaryCutoff * airScale);

    const auto coefficientFor = [sr] (float frequency) noexcept
    {
        return std::exp (-juce::MathConstants<float>::twoPi * frequency / sr);
    };

    coefficientA = coefficientFor (primaryCutoff);
    coefficientB = coefficientFor (secondaryCutoff);
    airCoefficient = coefficientFor (airCutoff);

    // Small compensation prevents perceived level collapse at high damping.
    outputCompensation = 1.0f + (0.08f * dampingAmount) + (0.04f * airLossAmount);
}

float Damping::process (float input, ChannelState& state) noexcept
{
    state.lowPassA += (input - state.lowPassA) * (1.0f - coefficientA);
    state.lowPassB += (state.lowPassA - state.lowPassB) * (1.0f - coefficientB);
    state.air += (state.lowPassB - state.air) * (1.0f - airCoefficient);

    // Blend cascaded stages to avoid an unnaturally steep low-pass response.
    const auto materialBlend = juce::jmap (dampingAmount,
                                           state.lowPassA,
                                           state.lowPassB);
    const auto airBlend = juce::jmap (airLossAmount,
                                      materialBlend,
                                      state.air);

    return airBlend * outputCompensation;
}

float Damping::processLeft (float input) noexcept
{
    return process (input, leftState);
}

float Damping::processRight (float input) noexcept
{
    return process (input, rightState);
}
