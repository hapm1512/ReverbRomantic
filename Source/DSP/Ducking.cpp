#include "Ducking.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float minimumDetector = 1.0e-9f;
constexpr float bypassFadeMs = 20.0f;
constexpr float lookAheadMs = 5.0f;
constexpr float rmsWindowMs = 18.0f;
constexpr float peakWeight = 0.38f;
constexpr float rmsWeight = 1.0f - peakWeight;
}

void Ducking::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = juce::jmax (1.0, spec.sampleRate);
    lookAheadSamples = juce::jmax (1, static_cast<int> (
        std::round (sampleRate * lookAheadMs * 0.001)));

    lookAheadLeft.assign (static_cast<size_t> (lookAheadSamples + 1), 0.0f);
    lookAheadRight.assign (static_cast<size_t> (lookAheadSamples + 1), 0.0f);

    updateTimeConstants();

    bypassCoefficient = std::exp (-1.0f /
                                  (0.001f * bypassFadeMs
                                   * static_cast<float> (sampleRate)));
    reset();
}

void Ducking::reset() noexcept
{
    peakEnvelope = 0.0f;
    rmsPower = 0.0f;
    hybridEnvelope = 0.0f;
    previousHybridEnvelope = 0.0f;
    smoothedGain = 1.0f;
    enabledTarget = enabled ? 1.0f : 0.0f;
    enabledMix = enabledTarget;
    lookAheadWriteIndex = 0;

    std::fill (lookAheadLeft.begin(), lookAheadLeft.end(), 0.0f);
    std::fill (lookAheadRight.begin(), lookAheadRight.end(), 0.0f);
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
    juce::ScopedNoDenormals noDenormals;

    const float linkedPeak = juce::jmax (std::abs (dryL), std::abs (dryR));
    const float linkedPower = 0.5f * (dryL * dryL + dryR * dryR);

    const float peakCoefficient = linkedPeak > peakEnvelope
                                    ? detectorAttackCoefficient
                                    : detectorReleaseCoefficient;
    peakEnvelope = linkedPeak
                 + peakCoefficient * (peakEnvelope - linkedPeak);

    rmsPower = linkedPower + rmsCoefficient * (rmsPower - linkedPower);
    const float rmsEnvelope = std::sqrt (juce::jmax (0.0f, rmsPower));

    previousHybridEnvelope = hybridEnvelope;
    const float hybridTarget = rmsWeight * rmsEnvelope + peakWeight * peakEnvelope;
    const float hybridCoefficient = hybridTarget > hybridEnvelope
                                      ? detectorAttackCoefficient
                                      : getAdaptiveReleaseCoefficient (hybridTarget,
                                                                       previousHybridEnvelope);
    hybridEnvelope = hybridTarget
                   + hybridCoefficient * (hybridEnvelope - hybridTarget);

    const float detectorDb = juce::Decibels::gainToDecibels (
        juce::jmax (hybridEnvelope, minimumDetector), -120.0f);

    const float targetGain = computeTargetGain (detectorDb);
    const float gainCoefficient = targetGain < smoothedGain
                                    ? gainAttackCoefficient
                                    : getAdaptiveReleaseCoefficient (hybridEnvelope,
                                                                     previousHybridEnvelope);
    smoothedGain = targetGain
                 + gainCoefficient * (smoothedGain - targetGain);

    enabledMix = enabledTarget
               + bypassCoefficient * (enabledMix - enabledTarget);

    float delayedWetL = wetL;
    float delayedWetR = wetR;

    if (! lookAheadLeft.empty())
    {
        delayedWetL = lookAheadLeft[static_cast<size_t> (lookAheadWriteIndex)];
        delayedWetR = lookAheadRight[static_cast<size_t> (lookAheadWriteIndex)];
        lookAheadLeft[static_cast<size_t> (lookAheadWriteIndex)] = wetL;
        lookAheadRight[static_cast<size_t> (lookAheadWriteIndex)] = wetR;

        if (++lookAheadWriteIndex >= static_cast<int> (lookAheadLeft.size()))
            lookAheadWriteIndex = 0;
    }

    // Crossfade into the delayed wet path only while Ducking is active.
    const float lookAheadWetL = juce::jmap (enabledMix, wetL, delayedWetL);
    const float lookAheadWetR = juce::jmap (enabledMix, wetR, delayedWetR);
    const float appliedGain = juce::jmap (enabledMix, 1.0f, smoothedGain);

    wetL = lookAheadWetL * appliedGain;
    wetR = lookAheadWetR * appliedGain;

    if (std::abs (peakEnvelope) < 1.0e-20f)
        peakEnvelope = 0.0f;
    if (std::abs (rmsPower) < 1.0e-30f)
        rmsPower = 0.0f;
    if (std::abs (hybridEnvelope) < 1.0e-20f)
        hybridEnvelope = 0.0f;
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
                         (0.001f * juce::jmax (0.1f, milliseconds)
                          * static_cast<float> (sampleRate)));
    };

    detectorAttackCoefficient = coefficientForMs (juce::jmax (0.5f, attackMs * 0.45f));
    detectorReleaseCoefficient = coefficientForMs (juce::jmax (40.0f, releaseMs * 0.65f));
    gainAttackCoefficient = coefficientForMs (attackMs);
    gainReleaseCoefficient = coefficientForMs (releaseMs);
    rmsCoefficient = coefficientForMs (rmsWindowMs);
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
        const float position = juce::jlimit (
            0.0f, 1.0f,
            (detectorDb - (thresholdDb - halfKnee)) / kneeDb);
        activity = position * position * (3.0f - 2.0f * position);
    }

    // The user Depth parameter remains the maximum gain reduction.
    return juce::jlimit (1.0f - depthLinear, 1.0f,
                         1.0f - depthLinear * activity);
}

float Ducking::getAdaptiveReleaseCoefficient (float detector,
                                              float previousDetector) const noexcept
{
    const float safePrevious = juce::jmax (previousDetector, minimumDetector);
    const float fallingRatio = juce::jlimit (0.0f, 1.0f,
                                             detector / safePrevious);

    // Fast recovery after short peaks, slower recovery during sustained vocals.
    const float activity = juce::jlimit (0.0f, 1.0f,
        juce::Decibels::gainToDecibels (juce::jmax (detector, minimumDetector), -120.0f)
            / juce::jmax (-1.0f, thresholdDb));
    const float adaptiveMs = juce::jmap (0.55f * activity + 0.45f * fallingRatio,
                                         releaseMs * 0.55f,
                                         releaseMs * 1.35f);

    return std::exp (-1.0f /
                     (0.001f * juce::jmax (20.0f, adaptiveMs)
                      * static_cast<float> (sampleRate)));
}
