#include "EarlyReflection.h"
#include <cmath>

void EarlyReflection::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = juce::jmax (8000.0, spec.sampleRate);

    // Maximum room scale is 2.0, plus stereo offsets and interpolation margin.
    const auto requiredSamples = static_cast<size_t> (std::ceil (sampleRate * 0.40)) + 8u;
    bufferL.assign (requiredSamples, 0.0f);
    bufferR.assign (requiredSamples, 0.0f);

    updateTapDelays();
    reset();
}

void EarlyReflection::reset()
{
    std::fill (bufferL.begin(), bufferL.end(), 0.0f);
    std::fill (bufferR.begin(), bufferR.end(), 0.0f);
    writeIndex = 0;
}

void EarlyReflection::setSize (float newSize) noexcept
{
    size = juce::jlimit (0.25f, 2.0f, newSize);
    updateTapDelays();
}

void EarlyReflection::updateTapDelays() noexcept
{
    const float samplesPerMs = static_cast<float> (sampleRate * 0.001);

    for (size_t i = 0; i < numTaps; ++i)
    {
        const float groupOffset = static_cast<float> (i / 8u) * 0.37f;
        const float leftMs = (delayMs[i] + groupOffset) * size;

        // Independent right-wall geometry avoids mirrored comb patterns.
        const float stereoRatio = ((i & 1u) == 0u) ? 1.037f : 0.971f;
        const float rightMs = (delayMs[i] * stereoRatio + 0.53f + groupOffset) * size;

        delaySamplesL[i] = leftMs * samplesPerMs;
        delaySamplesR[i] = rightMs * samplesPerMs;
    }
}

float EarlyReflection::readInterpolated (const std::vector<float>& buffer,
                                         float delaySamples) const noexcept
{
    if (buffer.empty())
        return 0.0f;

    const float bufferSize = static_cast<float> (buffer.size());
    float readPosition = static_cast<float> (writeIndex) - delaySamples;

    while (readPosition < 0.0f)
        readPosition += bufferSize;

    while (readPosition >= bufferSize)
        readPosition -= bufferSize;

    const auto indexA = static_cast<size_t> (readPosition);
    const auto indexB = (indexA + 1u) % buffer.size();
    const float fraction = readPosition - static_cast<float> (indexA);

    return buffer[indexA] + fraction * (buffer[indexB] - buffer[indexA]);
}

void EarlyReflection::processStereo (float inputL, float inputR,
                                     float& outputL, float& outputR) noexcept
{
    if (bufferL.empty() || bufferR.empty())
    {
        outputL = 0.0f;
        outputR = 0.0f;
        return;
    }

    // Small orthogonal-style crossfeed creates distinct wall excitation.
    bufferL[writeIndex] = inputL + inputR * 0.115f;
    bufferR[writeIndex] = inputR - inputL * 0.115f;

    float sumL = 0.0f;
    float sumR = 0.0f;

    for (size_t i = 0; i < numTaps; ++i)
    {
        const float reflectedL = readInterpolated (bufferL, delaySamplesL[i]);
        const float reflectedR = readInterpolated (bufferR, delaySamplesR[i]);

        // Four groups approximate nearby walls, floor/ceiling and rear surfaces.
        const float cross = (i >= 16u) ? 0.085f : 0.045f;
        sumL += reflectedL * gainL[i] + reflectedR * gainR[i] * cross;
        sumR += reflectedR * gainR[i] + reflectedL * gainL[i] * cross;
    }

    outputL = sumL * outputGain;
    outputR = sumR * outputGain;

    if (++writeIndex >= bufferL.size())
        writeIndex = 0;
}
