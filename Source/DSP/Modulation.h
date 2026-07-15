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

    float nextValue() noexcept;
    float getOffset (int line) const noexcept;

private:
    static constexpr int tableSize = 2048;
    static constexpr int tableMask = tableSize - 1;
    static_assert ((tableSize & tableMask) == 0,
                   "Sine table size must be a power of two");

    float readSine (float phase) const noexcept;
    float nextNoise (std::uint32_t& state) noexcept;

    double sampleRate = 44100.0;
    float amount = 0.22f;

    std::array<float, tableSize + 1> sineTable {};
    std::array<float, lineCount> phases {};
    std::array<float, lineCount> phaseIncrements {};
    std::array<float, lineCount> secondaryPhases {};
    std::array<float, lineCount> secondaryIncrements {};
    std::array<float, lineCount> randomWalk {};
    std::array<float, lineCount> randomVelocity {};
    std::array<float, lineCount> lineOffsets {};
    std::array<std::uint32_t, lineCount> noiseStates {};

    float globalDriftPhase = 0.0f;
    float globalDriftIncrement = 0.0f;
    float walkDamping = 0.0f;
    float walkExcitation = 0.0f;
};
