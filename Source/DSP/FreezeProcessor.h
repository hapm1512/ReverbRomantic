#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

// Stereo freeze/hold processor.
// Captures a rolling wet tail and crossfades into a protected looping buffer.
class FreezeProcessor
{
public:
    struct Parameters
    {
        bool enabled = false;
        float fadeTimeMs = 80.0f;
        float damping = 0.35f;
        float mixPercent = 100.0f;
    };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void setParameters (const Parameters& newParameters) noexcept;

    void processStereo (float inputL, float inputR,
                        float& outputL, float& outputR) noexcept;

    [[nodiscard]] bool isFrozen() const noexcept { return parameters.enabled; }

private:
    static constexpr std::size_t channelCount = 2u;

    [[nodiscard]] float readLoopSample (std::size_t channel) const noexcept;
    [[nodiscard]] float processDamping (float input,
                                        std::size_t channel,
                                        float coefficient) noexcept;
    [[nodiscard]] static float softProtect (float input) noexcept;
    void captureFreezeBuffer() noexcept;
    void updateFadeRamp() noexcept;

    Parameters parameters;

    std::array<std::vector<float>, channelCount> history;
    std::array<std::vector<float>, channelCount> frozen;
    std::array<float, channelCount> dampingState { 0.0f, 0.0f };

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> freezeBlend;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dampingSmoother;

    double sampleRate = 44100.0;
    std::size_t historyWritePosition = 0u;
    std::size_t loopReadPosition = 0u;
    std::size_t loopLengthSamples = 1u;
    bool previousEnabled = false;
};
