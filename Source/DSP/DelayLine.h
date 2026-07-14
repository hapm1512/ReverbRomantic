#pragma once

#include <cstddef>
#include <vector>

// Realtime-safe fractional delay line.
// Memory is allocated only by prepare(); process() performs no allocation.
class RomanticDelayLine
{
public:
    void prepare (double sampleRate, double maximumDelaySeconds);
    void reset();

    // Sets the base delay. Changes are smoothed inside process().
    void setDelaySamples (float newDelaySamples) noexcept;

    // Optional per-sample modulation hook, expressed in samples.
    void setModulationOffset (float offsetSamples) noexcept;

    // Writes input and returns the fractionally delayed sample.
    float process (float input) noexcept;

    [[nodiscard]] float getDelaySamples() const noexcept { return targetDelaySamples; }
    [[nodiscard]] std::size_t getCapacity() const noexcept { return buffer.size(); }

private:
    [[nodiscard]] float readCubic (float delayInSamples) const noexcept;
    [[nodiscard]] std::size_t wrapIndex (std::ptrdiff_t index) const noexcept;
    static std::size_t nextPowerOfTwo (std::size_t value) noexcept;

    std::vector<float> buffer;
    std::size_t writePosition = 0;
    std::size_t indexMask = 0;

    float targetDelaySamples = 1.0f;
    float currentDelaySamples = 1.0f;
    float modulationOffsetSamples = 0.0f;
    float maximumDelaySamples = 1.0f;

    // Fast enough to follow reverb modulation, slow enough to avoid clicks.
    float delaySmoothingCoefficient = 1.0f;
};
