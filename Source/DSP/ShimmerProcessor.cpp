#include "ShimmerProcessor.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float maximumFeedback = 0.70f;
constexpr float minimumToneHz = 500.0f;
constexpr float maximumToneHz = 12000.0f;
constexpr float smoothingSeconds = 0.040f;
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

    reset();
    setParameters (parameters);
}

void ShimmerProcessor::reset() noexcept
{
    pitchShifter.reset();
    toneStateL = 0.0f;
    toneStateR = 0.0f;
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

float ShimmerProcessor::softLimit (float input) noexcept
{
    // Bounded, smooth protection for the internal feedback path.
    return std::tanh (std::clamp (input, -4.0f, 4.0f));
}

void ShimmerProcessor::processStereo (float inputL, float inputR,
                                      float& outputL, float& outputR) noexcept
{
    updateToneCoefficient (toneSmoother.getNextValue());

    const float active = bypassSmoother.getNextValue();
    const float mix = mixSmoother.getNextValue() * active;
    const float feedbackAmount = feedbackSmoother.getNextValue() * active;

    float shiftedL = 0.0f;
    float shiftedR = 0.0f;
    pitchShifter.processSample (inputL + feedbackL,
                                inputR + feedbackR,
                                shiftedL,
                                shiftedR);

    shiftedL = processToneFilter (shiftedL, toneStateL);
    shiftedR = processToneFilter (shiftedR, toneStateR);

    feedbackL = softLimit (shiftedL * feedbackAmount);
    feedbackR = softLimit (shiftedR * feedbackAmount);

    outputL = inputL + shiftedL * mix;
    outputR = inputR + shiftedR * mix;

    if (! std::isfinite (outputL) || ! std::isfinite (outputR))
    {
        outputL = std::isfinite (inputL) ? inputL : 0.0f;
        outputR = std::isfinite (inputR) ? inputR : 0.0f;
        feedbackL = 0.0f;
        feedbackR = 0.0f;
    }
}
