#include "ShimmerProcessor.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float maximumFeedback = 0.70f;
constexpr float minimumToneHz = 500.0f;
constexpr float maximumToneHz = 12000.0f;
constexpr float smoothingSeconds = 0.040f;
constexpr float decorrelationMix = 0.18f;
constexpr float feedbackCrossMix = 0.12f;
constexpr float shimmerOutputTrim = 0.82f;
}

void ShimmerProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = std::max (1.0, spec.sampleRate);
    pitchShifter.prepare (sampleRate, static_cast<int> (spec.maximumBlockSize));

    mixSmoother.reset (sampleRate, smoothingSeconds);
    feedbackSmoother.reset (sampleRate, smoothingSeconds);
    bypassSmoother.reset (sampleRate, 0.025);
    toneSmoother.reset (sampleRate, smoothingSeconds);

    mixSmoother.setCurrentAndTargetValue (0.0f);
    feedbackSmoother.setCurrentAndTargetValue (0.0f);
    bypassSmoother.setCurrentAndTargetValue (0.0f);
    toneSmoother.setCurrentAndTargetValue (parameters.toneHz);

    const std::array<float, 2> decorrelationTimesMs { 13.7f, 19.3f };
    for (std::size_t channel = 0; channel < decorrelators.size(); ++channel)
    {
        auto& delay = decorrelators[channel];
        delay.delaySamples = static_cast<std::size_t> (std::max (
            1.0, std::round (sampleRate * decorrelationTimesMs[channel] * 0.001)));
        delay.buffer.assign (delay.delaySamples + 1u, 0.0f);
        delay.writePosition = 0u;
    }

    // Approximately 18 Hz DC blocker at common sample rates.
    dcBlockCoefficient = std::exp (-juce::MathConstants<float>::twoPi
                                   * 18.0f / static_cast<float> (sampleRate));

    reset();
    setParameters (parameters);
}

void ShimmerProcessor::reset() noexcept
{
    pitchShifter.reset();

    for (auto& delay : decorrelators)
    {
        std::fill (delay.buffer.begin(), delay.buffer.end(), 0.0f);
        delay.writePosition = 0u;
    }

    toneStateL = 0.0f;
    toneStateR = 0.0f;
    dcInputL = dcInputR = 0.0f;
    dcOutputL = dcOutputR = 0.0f;
    feedbackL = 0.0f;
    feedbackR = 0.0f;
}

void ShimmerProcessor::setParameters (const Parameters& newParameters) noexcept
{
    parameters.enabled = newParameters.enabled;
    parameters.mixPercent = std::clamp (newParameters.mixPercent, 0.0f, 100.0f);
    parameters.pitchSemitones = std::clamp (newParameters.pitchSemitones, 0.0f, 12.0f);
    parameters.feedbackPercent = std::clamp (newParameters.feedbackPercent, 0.0f, 70.0f);
    parameters.toneHz = std::clamp (newParameters.toneHz, minimumToneHz, maximumToneHz);

    pitchShifter.setPitchSemitones (parameters.pitchSemitones);
    mixSmoother.setTargetValue (parameters.mixPercent * 0.01f);
    feedbackSmoother.setTargetValue (
        std::min (maximumFeedback, parameters.feedbackPercent * 0.01f));
    bypassSmoother.setTargetValue (parameters.enabled ? 1.0f : 0.0f);
    toneSmoother.setTargetValue (parameters.toneHz);
}

void ShimmerProcessor::updateToneCoefficient (float toneHz) noexcept
{
    const float safeTone = std::clamp (toneHz,
                                       minimumToneHz,
                                       std::min (maximumToneHz,
                                                 static_cast<float> (sampleRate * 0.45)));
    toneCoefficient = 1.0f - std::exp (
        -juce::MathConstants<float>::twoPi * safeTone / static_cast<float> (sampleRate));
}

float ShimmerProcessor::processToneFilter (float input, float& state) noexcept
{
    state += toneCoefficient * (input - state);
    state = std::isfinite (state) ? state : 0.0f;
    return state;
}

float ShimmerProcessor::processDcBlocker (float input,
                                           float& previousInput,
                                           float& previousOutput) const noexcept
{
    const float output = input - previousInput + dcBlockCoefficient * previousOutput;
    previousInput = input;
    previousOutput = std::isfinite (output) ? output : 0.0f;
    return previousOutput;
}

float ShimmerProcessor::processDecorrelator (Decorrelator& delay, float input) noexcept
{
    if (delay.buffer.empty())
        return input;

    const std::size_t readPosition = (delay.writePosition + 1u) % delay.buffer.size();
    const float delayed = delay.buffer[readPosition];
    delay.buffer[delay.writePosition] = std::isfinite (input) ? input : 0.0f;
    delay.writePosition = readPosition;
    return delayed;
}

float ShimmerProcessor::softLimit (float input) noexcept
{
    // Smooth bounded protection without a hard-knee feedback discontinuity.
    return std::tanh (std::clamp (input, -4.0f, 4.0f));
}

void ShimmerProcessor::processStereo (float inputL, float inputR,
                                      float& outputL, float& outputR) noexcept
{
    juce::ScopedNoDenormals noDenormals;

    // Pitch shifting is the most expensive optional stage. Once the bypass
    // ramp reaches silence, avoid running it until shimmer is enabled again.
    if (! parameters.enabled
        && ! bypassSmoother.isSmoothing()
        && bypassSmoother.getCurrentValue() <= 0.000001f)
    {
        outputL = inputL;
        outputR = inputR;
        feedbackL = 0.0f;
        feedbackR = 0.0f;
        return;
    }

    updateToneCoefficient (toneSmoother.getNextValue());

    const float active = bypassSmoother.getNextValue();
    const float mix = mixSmoother.getNextValue() * active;
    const float feedbackAmount = feedbackSmoother.getNextValue() * active;

    const float feedbackInputL = feedbackL * (1.0f - feedbackCrossMix)
                               + feedbackR * feedbackCrossMix;
    const float feedbackInputR = feedbackR * (1.0f - feedbackCrossMix)
                               - feedbackL * feedbackCrossMix;

    float shiftedL = 0.0f;
    float shiftedR = 0.0f;
    pitchShifter.processSample (inputL + feedbackInputL,
                                inputR + feedbackInputR,
                                shiftedL,
                                shiftedR);

    shiftedL = processToneFilter (shiftedL, toneStateL);
    shiftedR = processToneFilter (shiftedR, toneStateR);

    shiftedL = processDcBlocker (shiftedL, dcInputL, dcOutputL);
    shiftedR = processDcBlocker (shiftedR, dcInputR, dcOutputR);

    const float decorrelatedL = processDecorrelator (decorrelators[0], shiftedR);
    const float decorrelatedR = processDecorrelator (decorrelators[1], shiftedL);
    shiftedL += decorrelationMix * (decorrelatedL - shiftedL);
    shiftedR += decorrelationMix * (decorrelatedR - shiftedR);

    feedbackL = softLimit (shiftedL * feedbackAmount);
    feedbackR = softLimit (shiftedR * feedbackAmount);

    // Trim compensates dual-grain overlap and keeps perceived level stable.
    const float shimmerL = shiftedL * shimmerOutputTrim;
    const float shimmerR = shiftedR * shimmerOutputTrim;
    const float dryCompensation = 1.0f - 0.12f * mix;

    outputL = inputL * dryCompensation + shimmerL * mix;
    outputR = inputR * dryCompensation + shimmerR * mix;

    if (! std::isfinite (outputL) || ! std::isfinite (outputR))
    {
        outputL = std::isfinite (inputL) ? inputL : 0.0f;
        outputR = std::isfinite (inputR) ? inputR : 0.0f;
        feedbackL = 0.0f;
        feedbackR = 0.0f;
    }
}
