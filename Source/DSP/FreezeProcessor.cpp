#include "FreezeProcessor.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float minimumFadeMs = 20.0f;
constexpr float maximumFadeMs = 500.0f;
constexpr float minimumLoopSeconds = 0.35f;
constexpr float maximumLoopSeconds = 1.50f;
constexpr float smoothingSeconds = 0.040f;
constexpr float energyFloor = 1.0e-7f;
constexpr float maximumNormaliseGain = 1.35f;
constexpr float loopDecay = 0.999995f;
}

void FreezeProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = std::max (1.0, spec.sampleRate);

    const auto requestedLength = static_cast<std::size_t> (
        std::round (sampleRate * maximumLoopSeconds));
    loopLengthSamples = std::max<std::size_t> (
        static_cast<std::size_t> (sampleRate * minimumLoopSeconds),
        requestedLength);

    for (auto& channel : history)
        channel.assign (loopLengthSamples, 0.0f);

    for (auto& channel : frozen)
        channel.assign (loopLengthSamples, 0.0f);

    freezeBlend.reset (sampleRate, parameters.fadeTimeMs * 0.001);
    mixSmoother.reset (sampleRate, smoothingSeconds);
    dampingSmoother.reset (sampleRate, smoothingSeconds);

    reset();
    setParameters (parameters);
}

void FreezeProcessor::reset() noexcept
{
    for (auto& channel : history)
        std::fill (channel.begin(), channel.end(), 0.0f);

    for (auto& channel : frozen)
        std::fill (channel.begin(), channel.end(), 0.0f);

    dampingState.fill (0.0f);
    historyWritePosition = 0u;
    loopReadPosition = 0u;
    previousEnabled = false;

    freezeBlend.setCurrentAndTargetValue (0.0f);
    mixSmoother.setCurrentAndTargetValue (parameters.mixPercent * 0.01f);
    dampingSmoother.setCurrentAndTargetValue (parameters.damping);
}

void FreezeProcessor::setParameters (const Parameters& newParameters) noexcept
{
    const float newFadeTimeMs = std::clamp (newParameters.fadeTimeMs,
                                            minimumFadeMs,
                                            maximumFadeMs);
    const bool fadeTimeChanged = std::abs (newFadeTimeMs - parameters.fadeTimeMs) > 0.01f;

    parameters.enabled = newParameters.enabled;
    parameters.fadeTimeMs = newFadeTimeMs;
    parameters.damping = std::clamp (newParameters.damping, 0.0f, 1.0f);
    parameters.mixPercent = std::clamp (newParameters.mixPercent, 0.0f, 100.0f);

    if (fadeTimeChanged)
        freezeBlend.reset (sampleRate, parameters.fadeTimeMs * 0.001);

    freezeBlend.setTargetValue (parameters.enabled ? 1.0f : 0.0f);
    mixSmoother.setTargetValue (parameters.mixPercent * 0.01f);
    dampingSmoother.setTargetValue (parameters.damping);
}

void FreezeProcessor::captureFreezeBuffer() noexcept
{
    if (loopLengthSamples == 0u)
        return;

    double inputEnergy = 0.0;

    for (std::size_t index = 0; index < loopLengthSamples; ++index)
    {
        const std::size_t sourceIndex = (historyWritePosition + index) % loopLengthSamples;

        for (std::size_t channel = 0; channel < channelCount; ++channel)
        {
            const float sample = history[channel][sourceIndex];
            frozen[channel][index] = std::isfinite (sample) ? sample : 0.0f;
            inputEnergy += static_cast<double> (sample) * static_cast<double> (sample);
        }
    }

    const double sampleCount = static_cast<double> (loopLengthSamples * channelCount);
    const float rms = static_cast<float> (std::sqrt (inputEnergy / std::max (1.0, sampleCount)));
    const float targetRms = std::clamp (rms, 0.03f, 0.22f);
    const float gain = std::clamp (targetRms / std::max (rms, energyFloor),
                                   0.0f,
                                   maximumNormaliseGain);

    for (auto& channel : frozen)
        for (auto& sample : channel)
            sample = softProtect (sample * gain);

    loopReadPosition = 0u;
    dampingState.fill (0.0f);
}

void FreezeProcessor::updateFadeRamp() noexcept
{
    if (parameters.enabled != previousEnabled)
    {
        if (parameters.enabled)
            captureFreezeBuffer();

        freezeBlend.reset (sampleRate, parameters.fadeTimeMs * 0.001);
        freezeBlend.setTargetValue (parameters.enabled ? 1.0f : 0.0f);
        previousEnabled = parameters.enabled;
    }
}

float FreezeProcessor::readLoopSample (std::size_t channel) const noexcept
{
    if (channel >= channelCount || frozen[channel].empty())
        return 0.0f;

    return frozen[channel][loopReadPosition % frozen[channel].size()];
}

float FreezeProcessor::processDamping (float input,
                                        std::size_t channel,
                                        float coefficient) noexcept
{
    auto& state = dampingState[channel];
    state += coefficient * (input - state);
    state = std::isfinite (state) ? state : 0.0f;
    return state;
}

float FreezeProcessor::softProtect (float input) noexcept
{
    return std::tanh (std::clamp (input, -3.0f, 3.0f));
}

void FreezeProcessor::processStereo (float inputL, float inputR,
                                     float& outputL, float& outputR) noexcept
{
    juce::ScopedNoDenormals noDenormals;

    if (loopLengthSamples == 0u)
    {
        outputL = inputL;
        outputR = inputR;
        return;
    }

    history[0][historyWritePosition] = std::isfinite (inputL) ? inputL : 0.0f;
    history[1][historyWritePosition] = std::isfinite (inputR) ? inputR : 0.0f;
    historyWritePosition = (historyWritePosition + 1u) % loopLengthSamples;

    updateFadeRamp();

    const float damping = dampingSmoother.getNextValue();
    const float cutoffHz = juce::jmap (damping, 0.0f, 1.0f, 18000.0f, 3500.0f);
    const float coefficient = 1.0f - std::exp (
        -juce::MathConstants<float>::twoPi * cutoffHz / static_cast<float> (sampleRate));

    float frozenL = processDamping (readLoopSample (0u), 0u, coefficient);
    float frozenR = processDamping (readLoopSample (1u), 1u, coefficient);

    frozenL = softProtect (frozenL * loopDecay);
    frozenR = softProtect (frozenR * loopDecay);

    frozen[0][loopReadPosition] = frozenL;
    frozen[1][loopReadPosition] = frozenR;
    loopReadPosition = (loopReadPosition + 1u) % loopLengthSamples;

    const float blend = freezeBlend.getNextValue();
    const float mix = mixSmoother.getNextValue();
    const float heldL = inputL + (frozenL - inputL) * blend;
    const float heldR = inputR + (frozenR - inputR) * blend;

    outputL = inputL + (heldL - inputL) * mix;
    outputR = inputR + (heldR - inputR) * mix;

    if (! std::isfinite (outputL) || ! std::isfinite (outputR))
    {
        outputL = std::isfinite (inputL) ? inputL : 0.0f;
        outputR = std::isfinite (inputR) ? inputR : 0.0f;
        dampingState.fill (0.0f);
    }
}
