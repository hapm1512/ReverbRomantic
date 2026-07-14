#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstdint>

class Modulation
{
public:
    static constexpr int lineCount = 16;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;

    void setAmount (float percent) noexcept;

    // Advances the modulation engine by one sample.
    // Kept for compatibility with the current HybridFDN16 implementation.
    float nextValue() noexcept;

    // Per-line bipolar modulation value in the range approximately [-1, 1].
    // HybridFDN16 will consume this API in Epic 3B2C.
    float getOffset (int line) const noexcept;

private:
    static constexpr int tableSize = 2048;
    static constexpr int tableMask = tableSize - 1;
    static_assert ((tableSize & tableMask) == 0, "Sine table size must be a power of two");

    float readSine (float phase) const noexcept;
    float nextNoise() noexcept;
    void updateRandomTargets() noexcept;

    double sampleRate = 44100.0;
    float amount = 0.22f;

    std::array<float, tableSize + 1> sineTable {};
    std::array<float, 4> phases {};
    std::array<float, 4> phaseIncrements {};
    std::array<float, lineCount> phaseOffsets {};
    std::array<float, lineCount> lineOffsets {};
    std::array<float, lineCount> randomCurrent {};
    std::array<float, lineCount> randomTarget {};

    float driftPhase = 0.0f;
    float driftIncrement = 0.0f;
    float randomSmoothing = 0.0f;

    int randomUpdateCounter = 0;
    int randomUpdateInterval = 1;
    std::uint32_t randomState = 0x6d2b79f5u;
};
