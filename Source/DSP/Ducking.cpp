#include "Ducking.h"

#include <cmath>

namespace
{
constexpr float minimumDetector = 1.0e-9f;
constexpr float bypassFadeMs = 20.0f;
}

void Ducking::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = juce::jmax (1.0, spec.sampleRate);
    updateTimeConstants();

    bypassCoefficient = std::exp (-1.0f /
                                  (0.001f * bypassFadeMs
                                   * static_cast<float> (sampleRate)));
    reset();
}

void Ducking::reset() noexcept
{
    detectorEnvelope = 0.0f;
    smoothedGain = 1.0f;
    enabledTarget = enabled ? 1.0f : 0.0f;
    enabledMix = enabledTarget;
}

void Ducking::setEnabled (bool shouldEnable) noexcept
{
    enabled = shouldEnable;
    enabledTarget = enabled ? 1.0f : 0.0f;
}

void Ducking::setThresholdDb (float decibels) noexcept
{
    thresholdDb = juce::jlimit (-60.0f, 0.0f, decibels);
}

void Ducking::setDepth (float percent) noexcept
{
    depthLinear = juce::jlimit (0.0f, 1.0f, percent * 0.01f);
}

void Ducking::setAttackMs (float milliseconds) noexcept
{
    attackMs = juce::jlimit (1.0f, 100.0f, milliseconds);
    updateTimeConstants();
}

void Ducking::setReleaseMs (float milliseconds) noexcept
{
    releaseMs = juce::jlimit (50.0f, 1000.0f, milliseconds);
    updateTimeConstants();
}

void Ducking::setKneeDb (float decibels) noexcept
{
    kneeDb = juce::jlimit (0.0f, 12.0f, decibels);
}

void Ducking::setAmount (float percent) noexcept
{
    setDepth (percent);
}

void Ducking::process (float dryL, float dryR,
                       float& wetL, float& wetR) noexcept
{
    const float detector = juce::jmax (std::abs (dryL), std::abs (dryR));
    const float detectorCoefficient = detector > detectorEnvelope
                                        ? attackCoefficient
                                        : releaseCoefficient;

    detectorEnvelope = detector
                     + detectorCoefficient * (detectorEnvelope - detector);

    const float detectorDb = juce::Decibels::gainToDecibels (
        juce::jmax (detectorEnvelope, minimumDetector), -120.0f);

    const float targetGain = computeTargetGain (detectorDb);
    const float gainCoefficient = targetGain < smoothedGain
                                    ? attackCoefficient
                                    : releaseCoefficient;

    smoothedGain = targetGain
                 + gainCoefficient * (smoothedGain - targetGain);

    enabledMix = enabledTarget
               + bypassCoefficient * (enabledMix - enabledTarget);

    const float appliedGain = juce::jmap (enabledMix, 1.0f, smoothedGain);
    wetL *= appliedGain;
    wetR *= appliedGain;

    // Avoid denormals during long reverb tails.
    if (std::abs (detectorEnvelope) < 1.0e-20f)
        detectorEnvelope = 0.0f;
}

float Ducking::getGainReductionDb() const noexcept
{
    return -juce::Decibels::gainToDecibels (
        juce::jmax (smoothedGain, minimumDetector), -120.0f);
}

void Ducking::updateTimeConstants() noexcept
{
    const auto coefficientForMs = [this] (float milliseconds)
    {
        return std::exp (-1.0f /
                         (0.001f * milliseconds
                          * static_cast<float> (sampleRate)));
    };

    attackCoefficient = coefficientForMs (attackMs);
    releaseCoefficient = coefficientForMs (releaseMs);
}

float Ducking::computeTargetGain (float detectorDb) const noexcept
{
    const float halfKnee = 0.5f * kneeDb;
    float activity = 0.0f;

    if (kneeDb <= 0.0001f)
    {
        activity = detectorDb > thresholdDb ? 1.0f : 0.0f;
    }
    else if (detectorDb <= thresholdDb - halfKnee)
    {
        activity = 0.0f;
    }
    else if (detectorDb >= thresholdDb + halfKnee)
    {
        activity = 1.0f;
    }
    else
    {
        const float position = (detectorDb - (thresholdDb - halfKnee)) / kneeDb;
        activity = position * position * (3.0f - 2.0f * position);
    }

    return 1.0f - depthLinear * activity;
}
