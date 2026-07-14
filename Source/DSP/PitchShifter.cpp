#include "PitchShifter.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr float minimumSemitones = 0.0f;
constexpr float maximumSemitones = 12.0f;
constexpr float minimumWindowMs = 35.0f;
constexpr float maximumWindowMs = 120.0f;
constexpr float defaultWindowMs = 80.0f;
constexpr float minimumDelaySamples = 2.0f;
constexpr float twoPi = juce::MathConstants<float>::twoPi;

std::size_t nextPowerOfTwo (std::size_t value) noexcept
{
    if (value <= 2u)
        return 2u;

    --value;
    for (std::size_t shift = 1u;
         shift < std::numeric_limits<std::size_t>::digits;
         shift <<= 1u)
    {
        value |= value >> shift;
    }

    return value + 1u;
}
}

void PitchShifter::prepare (double newSampleRate, int maximumBlockSize)
{
    juce::ignoreUnused (maximumBlockSize);

    sampleRate = std::max (1.0, newSampleRate);

    const auto maximumWindowSamples = static_cast<std::size_t> (
        std::ceil (sampleRate * maximumWindowMs * 0.001)) + 8u;
    const auto capacity = nextPowerOfTwo (std::max<std::size_t> (64u, maximumWindowSamples));

    for (auto& channel : channels)
        channel.buffer.assign (capacity, 0.0f);

    indexMask = capacity - 1u;
    targetWindowSamples = static_cast<float> (sampleRate * defaultWindowMs * 0.001);
    currentWindowSamples = targetWindowSamples;

    const auto smoothingSamples = std::max (1.0, sampleRate * 0.020);
    parameterSmoothing = static_cast<float> (1.0 - std::exp (-1.0 / smoothingSamples));

    reset();
    updateDerivedValues();
}

void PitchShifter::reset() noexcept
{
    for (auto& channel : channels)
    {
        std::fill (channel.buffer.begin(), channel.buffer.end(), 0.0f);
        channel.writePosition = 0u;
    }

    phase = 0.0f;
    currentSemitones = targetSemitones;
    currentWindowSamples = targetWindowSamples;
    updateDerivedValues();
}

void PitchShifter::setPitchSemitones (float semitones) noexcept
{
    if (std::isfinite (semitones))
        targetSemitones = std::clamp (semitones, minimumSemitones, maximumSemitones);
}

void PitchShifter::setWindowSizeMs (float windowMs) noexcept
{
    if (! std::isfinite (windowMs))
        return;

    const float safeWindowMs = std::clamp (windowMs, minimumWindowMs, maximumWindowMs);
    targetWindowSamples = static_cast<float> (sampleRate * safeWindowMs * 0.001);
}

float PitchShifter::wrapUnit (float value) noexcept
{
    value -= std::floor (value);
    return value;
}

float PitchShifter::raisedCosine (float grainPhase) noexcept
{
    // Equal complementary windows for the two grains.
    return 0.5f - 0.5f * std::cos (twoPi * wrapUnit (grainPhase));
}

void PitchShifter::updateDerivedValues() noexcept
{
    ratio = std::exp2 (currentSemitones / 12.0f);
    phaseIncrement = std::max (0.0f, ratio - 1.0f)
                     / std::max (32.0f, currentWindowSamples);
}

float PitchShifter::readLinear (const Channel& channel, float delaySamples) const noexcept
{
    if (channel.buffer.empty())
        return 0.0f;

    const float safeDelay = std::clamp (delaySamples,
                                        minimumDelaySamples,
                                        currentWindowSamples + minimumDelaySamples);
    const float readPosition = static_cast<float> (channel.writePosition) - safeDelay;
    const auto indexA = static_cast<std::ptrdiff_t> (std::floor (readPosition));
    const float fraction = readPosition - static_cast<float> (indexA);

    const float sampleA = channel.buffer[static_cast<std::size_t> (indexA) & indexMask];
    const float sampleB = channel.buffer[static_cast<std::size_t> (indexA + 1) & indexMask];
    return sampleA + fraction * (sampleB - sampleA);
}

float PitchShifter::processChannel (Channel& channel, float input) noexcept
{
    if (channel.buffer.empty())
        return input;

    channel.buffer[channel.writePosition] = std::isfinite (input) ? input : 0.0f;

    const float phaseA = phase;
    const float phaseB = wrapUnit (phase + 0.5f);
    const float delayA = minimumDelaySamples + phaseA * currentWindowSamples;
    const float delayB = minimumDelaySamples + phaseB * currentWindowSamples;
    const float windowA = raisedCosine (phaseA);
    const float windowB = raisedCosine (phaseB);

    const float normalisation = std::max (0.25f, windowA + windowB);
    const float output = (readLinear (channel, delayA) * windowA
                        + readLinear (channel, delayB) * windowB) / normalisation;

    channel.writePosition = (channel.writePosition + 1u) & indexMask;
    return output;
}

void PitchShifter::processSample (float inputL, float inputR,
                                  float& outputL, float& outputR) noexcept
{
    currentSemitones += (targetSemitones - currentSemitones) * parameterSmoothing;
    currentWindowSamples += (targetWindowSamples - currentWindowSamples) * parameterSmoothing;
    updateDerivedValues();

    outputL = processChannel (channels[0], inputL);
    outputR = processChannel (channels[1], inputR);

    phase = wrapUnit (phase + phaseIncrement);
}
