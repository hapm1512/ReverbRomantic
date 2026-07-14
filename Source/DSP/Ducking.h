#pragma once

#include <JuceHeader.h>

// Internal stereo ducking gain computer.
// The dry signal drives the detector; gain is applied only to wet reverb.
class Ducking
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;

    void setEnabled (bool shouldEnable) noexcept;
    void setThresholdDb (float decibels) noexcept;
    void setDepth (float percent) noexcept;
    void setAttackMs (float milliseconds) noexcept;
    void setReleaseMs (float milliseconds) noexcept;
    void setKneeDb (float decibels) noexcept;

    // Backward-compatible control used by HybridFDN16.
    void setAmount (float percent) noexcept;

    void process (float dryL, float dryR,
                  float& wetL, float& wetR) noexcept;

    [[nodiscard]] float getGainReductionDb() const noexcept;

private:
    void updateTimeConstants() noexcept;
    [[nodiscard]] float computeTargetGain (float detectorDb) const noexcept;

    double sampleRate = 44100.0;

    float thresholdDb = -24.0f;
    float depthLinear = 0.50f;
    float attackMs = 10.0f;
    float releaseMs = 250.0f;
    float kneeDb = 6.0f;

    float attackCoefficient = 0.0f;
    float releaseCoefficient = 0.0f;
    float detectorEnvelope = 0.0f;
    float smoothedGain = 1.0f;
    float enabledMix = 1.0f;
    float enabledTarget = 1.0f;
    float bypassCoefficient = 0.0f;

    bool enabled = true;
};
