#pragma once

#include <JuceHeader.h>
#include "PitchShifter.h"

// Stereo shimmer building block.
// It produces a protected pitch-shifted feedback texture without owning the FDN.
class ShimmerProcessor
{
public:
    struct Parameters
    {
        bool enabled = false;
        float mixPercent = 0.0f;
        float pitchSemitones = 12.0f;
        float feedbackPercent = 35.0f;
        float toneHz = 8000.0f;
    };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void setParameters (const Parameters& newParameters) noexcept;

    void processStereo (float inputL, float inputR,
                        float& outputL, float& outputR) noexcept;

    [[nodiscard]] float getFeedbackLeft() const noexcept { return feedbackL; }
    [[nodiscard]] float getFeedbackRight() const noexcept { return feedbackR; }

private:
    [[nodiscard]] float processToneFilter (float input, float& state) noexcept;
    [[nodiscard]] static float softLimit (float input) noexcept;
    void updateToneCoefficient (float toneHz) noexcept;

    PitchShifter pitchShifter;
    Parameters parameters;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> feedbackSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> toneSmoother;

    double sampleRate = 44100.0;
    float toneCoefficient = 0.5f;
    float toneStateL = 0.0f;
    float toneStateR = 0.0f;
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;
};
