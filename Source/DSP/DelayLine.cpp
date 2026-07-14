#include "DelayLine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr float minimumDelaySamples = 1.0f;
constexpr float smoothingTimeSeconds = 0.0025f;
}

std::size_t RomanticDelayLine::nextPowerOfTwo (std::size_t value) noexcept
{
    if (value <= 2u)
        return 2u;

    --value;
    for (std::size_t shift = 1u; shift < std::numeric_limits<std::size_t>::digits; shift <<= 1u)
        value |= value >> shift;

    return value + 1u;
}

void RomanticDelayLine::prepare (double sampleRate, double maximumDelaySeconds)
{
    const auto safeSampleRate = std::max (1.0, sampleRate);
    const auto safeSeconds = std::max (0.0, maximumDelaySeconds);

    // Four guard samples are sufficient for four-point cubic interpolation.
    const auto requestedSamples = static_cast<std::size_t> (
        std::ceil (safeSampleRate * safeSeconds)) + 4u;
    const auto capacity = nextPowerOfTwo (std::max<std::size_t> (8u, requestedSamples));

    buffer.assign (capacity, 0.0f);
    indexMask = capacity - 1u;
    maximumDelaySamples = static_cast<float> (capacity - 3u);

    const auto smoothingSamples = std::max (1.0, safeSampleRate * smoothingTimeSeconds);
    delaySmoothingCoefficient = static_cast<float> (1.0 - std::exp (-1.0 / smoothingSamples));

    targetDelaySamples = std::clamp (targetDelaySamples,
                                     minimumDelaySamples,
                                     maximumDelaySamples);
    reset();
}

void RomanticDelayLine::reset()
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writePosition = 0u;
    currentDelaySamples = targetDelaySamples;
    modulationOffsetSamples = 0.0f;
}

void RomanticDelayLine::setDelaySamples (float newDelaySamples) noexcept
{
    if (buffer.empty() || ! std::isfinite (newDelaySamples))
        return;

    targetDelaySamples = std::clamp (newDelaySamples,
                                     minimumDelaySamples,
                                     maximumDelaySamples);
}

void RomanticDelayLine::setModulationOffset (float offsetSamples) noexcept
{
    modulationOffsetSamples = std::isfinite (offsetSamples) ? offsetSamples : 0.0f;
}

std::size_t RomanticDelayLine::wrapIndex (std::ptrdiff_t index) const noexcept
{
    return static_cast<std::size_t> (index) & indexMask;
}

float RomanticDelayLine::readCubic (float delayInSamples) const noexcept
{
    const float readPosition = static_cast<float> (writePosition) - delayInSamples;
    const auto integerPosition = static_cast<std::ptrdiff_t> (std::floor (readPosition));
    const float fraction = readPosition - static_cast<float> (integerPosition);

    const float y0 = buffer[wrapIndex (integerPosition - 1)];
    const float y1 = buffer[wrapIndex (integerPosition)];
    const float y2 = buffer[wrapIndex (integerPosition + 1)];
    const float y3 = buffer[wrapIndex (integerPosition + 2)];

    // Catmull-Rom four-point cubic interpolation.
    const float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    const float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float a2 = -0.5f * y0 + 0.5f * y2;
    const float a3 = y1;

    return ((a0 * fraction + a1) * fraction + a2) * fraction + a3;
}

float RomanticDelayLine::process (float input) noexcept
{
    if (buffer.empty())
        return input;

    buffer[writePosition] = input;

    currentDelaySamples += (targetDelaySamples - currentDelaySamples)
                           * delaySmoothingCoefficient;

    const float effectiveDelay = std::clamp (currentDelaySamples + modulationOffsetSamples,
                                             minimumDelaySamples,
                                             maximumDelaySamples);
    const float output = readCubic (effectiveDelay);

    writePosition = (writePosition + 1u) & indexMask;
    return output;
}
