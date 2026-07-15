#pragma once

#include <JuceHeader.h>
#include <vector>

// Professional stereo-linked ducking gain computer.
// The dry signal drives a hybrid RMS/peak detector; gain is applied only to wet reverb.
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
    [[nodiscard]] float getAdaptiveReleaseCoefficient (float detector,
                                                       float previousDetector) const noexcept;

    double sampleRate = 44100.0;

    float thresholdDb = -24.0f;
    float depthLinear = 0.50f;
    float attackMs = 10.0f;
    float releaseMs = 250.0f;
    float kneeDb = 6.0f;

    float detectorAttackCoefficient = 0.0f;
    float detectorReleaseCoefficient = 0.0f;
    float gainAttackCoefficient = 0.0f;
    float gainReleaseCoefficient = 0.0f;
    float rmsCoefficient = 0.0f;
    float bypassCoefficient = 0.0f;

    float peakEnvelope = 0.0f;
    float rmsPower = 0.0f;
    float hybridEnvelope = 0.0f;
    float previousHybridEnvelope = 0.0f;
    float smoothedGain = 1.0f;
    float enabledMix = 0.0f;
    float enabledTarget = 0.0f;

    std::vector<float> lookAheadLeft;
    std::vector<float> lookAheadRight;
    int lookAheadWriteIndex = 0;
    int lookAheadSamples = 0;

    bool enabled = false;
};
