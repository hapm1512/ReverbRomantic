#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

// Low-latency dual-grain pitch shifter for shimmer feedback.
// Allocation is restricted to prepare(); processSample() is realtime-safe.
class PitchShifter
{
public:
    void prepare (double newSampleRate, int maximumBlockSize);
    void reset() noexcept;

    // Supported range is intentionally limited for stable shimmer operation.
    void setPitchSemitones (float semitones) noexcept;
    void setWindowSizeMs (float windowMs) noexcept;

    void processSample (float inputL, float inputR,
                        float& outputL, float& outputR) noexcept;

    [[nodiscard]] float getPitchSemitones() const noexcept
    {
        return targetSemitones;
    }

    [[nodiscard]] int getLatencySamples() const noexcept
    {
        return static_cast<int> (0.5f * currentWindowSamples);
    }

private:
    struct Channel
    {
        std::vector<float> buffer;
        std::size_t writePosition = 0u;
    };

    [[nodiscard]] float processChannel (Channel& channel, float input) noexcept;
    [[nodiscard]] float readLinear (const Channel& channel, float delaySamples) const noexcept;
    [[nodiscard]] static float raisedCosine (float phase) noexcept;
    [[nodiscard]] static float wrapUnit (float value) noexcept;

    void updateDerivedValues() noexcept;

    std::array<Channel, 2> channels;

    double sampleRate = 44100.0;
    float targetSemitones = 12.0f;
    float currentSemitones = 12.0f;
    float targetWindowSamples = 4096.0f;
    float currentWindowSamples = 4096.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float ratio = 2.0f;
    float parameterSmoothing = 0.001f;
    std::size_t indexMask = 0u;
};
