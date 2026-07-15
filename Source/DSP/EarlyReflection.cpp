#include "EarlyReflection.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr std::array<float, 32> geometrySeed
{
     1.000f, 1.137f, 1.291f, 1.463f, 1.657f, 1.877f, 2.123f, 2.401f,
     2.713f, 3.067f, 3.467f, 3.919f, 4.429f, 5.003f, 5.653f, 6.389f,
     7.219f, 8.157f, 9.217f,10.413f,11.763f,13.287f,15.009f,16.955f,
    19.151f,21.631f,24.433f,27.599f,31.175f,35.215f,39.779f,44.937f
};

constexpr std::array<float, 32> polarityL
{
     1,-1, 1, 1,-1, 1,-1, 1, 1,-1, 1,-1, 1, 1,-1, 1,
    -1, 1, 1,-1, 1,-1, 1, 1,-1, 1,-1, 1, 1,-1, 1,-1
};

constexpr std::array<float, 32> polarityR
{
    -1, 1, 1,-1, 1,-1, 1,-1, 1, 1,-1, 1,-1, 1, 1,-1,
     1,-1, 1, 1,-1, 1,-1, 1, 1,-1, 1,-1, 1, 1,-1, 1
};
}

EarlyReflection::ShapeProfile EarlyReflection::profileFor (Shape value) noexcept
{
    switch (value)
    {
        case Shape::room:      return { 0.72f, 0.68f, 0.62f, 0.42f, 0.34f, 0.055f, 0.94f };
        case Shape::studio:    return { 0.86f, 0.74f, 0.66f, 0.54f, 0.48f, 0.047f, 0.90f };
        case Shape::chamber:   return { 0.94f, 0.92f, 0.82f, 0.38f, 0.58f, 0.061f, 0.92f };
        case Shape::cathedral: return { 1.34f, 1.58f, 1.82f, 0.28f, 0.72f, 0.082f, 0.82f };
        case Shape::plate:     return { 1.12f, 0.54f, 0.18f, 0.18f, 0.82f, 0.074f, 0.78f };
        case Shape::hall:
        default:               return { 1.18f, 1.26f, 1.08f, 0.32f, 0.66f, 0.070f, 0.86f };
    }
}

void EarlyReflection::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = juce::jmax (8000.0, spec.sampleRate);
    const auto requiredSamples = static_cast<size_t> (std::ceil (sampleRate * 0.55)) + 8u;
    bufferL.assign (requiredSamples, 0.0f);
    bufferR.assign (requiredSamples, 0.0f);
    updateTapGeometry();
    updateAbsorption();
    reset();
}

void EarlyReflection::reset()
{
    std::fill (bufferL.begin(), bufferL.end(), 0.0f);
    std::fill (bufferR.begin(), bufferR.end(), 0.0f);
    filterStateL.fill (0.0f);
    filterStateR.fill (0.0f);
    diffusionStateL.fill (0.0f);
    diffusionStateR.fill (0.0f);
    writeIndex = 0;
}

void EarlyReflection::setSize (float newSize) noexcept
{
    size = juce::jlimit (0.25f, 2.35f, newSize);
    updateTapGeometry();
}

void EarlyReflection::setShape (Shape newShape) noexcept
{
    shape = newShape;
    profile = profileFor (shape);
    updateTapGeometry();
    updateAbsorption();
}

void EarlyReflection::setHighCut (float frequencyHz) noexcept
{
    highCutHz = juce::jlimit (1200.0f, 20000.0f, frequencyHz);
    updateAbsorption();
}

void EarlyReflection::updateTapGeometry() noexcept
{
    const float samplesPerMs = static_cast<float> (sampleRate * 0.001);
    constexpr float speedToMs = 2.91545f;

    for (size_t i = 0; i < numTaps; ++i)
    {
        const float group = static_cast<float> (i / 8u);
        const float axis = (i % 3u) == 0u ? profile.width
                         : (i % 3u) == 1u ? profile.depth
                                         : profile.height;
        const float distance = geometrySeed[i] * axis;
        const float baseMs = (2.65f + distance * speedToMs + group * 0.73f) * size;
        const float stereoSkew = ((i & 1u) == 0u ? 1.0f : -1.0f)
                                 * (0.19f + 0.013f * static_cast<float> (i));

        delaySamplesL[i] = juce::jmax (1.0f, (baseMs + stereoSkew) * samplesPerMs);
        delaySamplesR[i] = juce::jmax (1.0f, (baseMs - stereoSkew * 1.17f + 0.41f) * samplesPerMs);

        const float bounceRatio = 1.43f + 0.021f * static_cast<float> ((i * 7u) % 11u);
        secondaryDelayL[i] = delaySamplesL[i] * bounceRatio + samplesPerMs * (0.31f + group * 0.11f);
        secondaryDelayR[i] = delaySamplesR[i] * (bounceRatio + 0.027f) + samplesPerMs * (0.47f + group * 0.09f);

        const float distanceLoss = 1.0f / (1.0f + 0.115f * distance);
        const float groupLoss = 1.0f - 0.055f * group;
        gainL[i] = polarityL[i] * distanceLoss * groupLoss * 0.285f;
        gainR[i] = polarityR[i] * distanceLoss * groupLoss * 0.279f;
        secondaryGain[i] = (0.095f + profile.diffusion * 0.075f)
                           * std::pow (0.972f, static_cast<float> (i));
    }
}

void EarlyReflection::updateAbsorption() noexcept
{
    const float sampleRateFloat = static_cast<float> (sampleRate);

    for (size_t i = 0; i < numTaps; ++i)
    {
        const float distanceRatio = static_cast<float> (i) / static_cast<float> (numTaps - 1u);
        const float airLoss = juce::jmap (distanceRatio, 1.0f, 0.48f);
        const float materialLoss = juce::jlimit (0.20f, 1.0f, 1.0f - profile.absorption * distanceRatio);
        const float cutoff = juce::jlimit (900.0f, 20000.0f,
                                           highCutHz * airLoss * materialLoss);
        absorptionCoefficient[i] = std::exp (-juce::MathConstants<float>::twoPi
                                              * cutoff / sampleRateFloat);
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
    const auto indexB = indexA + 1u < buffer.size() ? indexA + 1u : 0u;
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

    bufferL[writeIndex] = inputL + inputR * profile.crossfeed;
    bufferR[writeIndex] = inputR - inputL * profile.crossfeed;

    float sumL = 0.0f;
    float sumR = 0.0f;

    for (size_t i = 0; i < numTaps; ++i)
    {
        const float primaryL = readInterpolated (bufferL, delaySamplesL[i]);
        const float primaryR = readInterpolated (bufferR, delaySamplesR[i]);
        const float bounceL = readInterpolated (bufferR, secondaryDelayL[i]);
        const float bounceR = readInterpolated (bufferL, secondaryDelayR[i]);

        const float rawL = primaryL + bounceL * secondaryGain[i];
        const float rawR = primaryR + bounceR * secondaryGain[i];
        const float coefficient = absorptionCoefficient[i];

        filterStateL[i] = rawL * (1.0f - coefficient) + filterStateL[i] * coefficient;
        filterStateR[i] = rawR * (1.0f - coefficient) + filterStateR[i] * coefficient;

        const float diffuseL = filterStateL[i]
                               + profile.diffusion * 0.115f
                                 * (diffusionStateR[i] - filterStateL[i]);
        const float diffuseR = filterStateR[i]
                               + profile.diffusion * 0.115f
                                 * (diffusionStateL[i] - filterStateR[i]);
        diffusionStateL[i] = diffuseL;
        diffusionStateR[i] = diffuseR;

        const float lateCross = i >= 16u ? 0.072f : 0.036f;
        sumL += diffuseL * gainL[i] + diffuseR * gainR[i] * lateCross;
        sumR += diffuseR * gainR[i] + diffuseL * gainL[i] * lateCross;
    }

    outputL = sumL * profile.output;
    outputR = sumR * profile.output;

    if (++writeIndex >= bufferL.size())
        writeIndex = 0;
}
