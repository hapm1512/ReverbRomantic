#include "EarlyReflection.h"

void EarlyReflection::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    for (size_t i = 0; i < numTaps; ++i)
    {
        tapsL[i].prepare (sampleRate, 0.15);
        tapsR[i].prepare (sampleRate, 0.15);
    }
    setSize (size);
    reset();
}

void EarlyReflection::reset()
{
    for (auto& tap : tapsL) tap.reset();
    for (auto& tap : tapsR) tap.reset();
}

void EarlyReflection::setSize (float newSize) noexcept
{
    size = juce::jlimit (0.25f, 2.0f, newSize);
    for (size_t i = 0; i < numTaps; ++i)
    {
        tapsL[i].setDelaySamples (tapMs[i] * size * 0.001f * static_cast<float> (sampleRate));
        tapsR[i].setDelaySamples ((tapMs[i] * 1.071f + 0.7f) * size * 0.001f * static_cast<float> (sampleRate));
    }
}

void EarlyReflection::processStereo (float inputL, float inputR, float& outputL, float& outputR) noexcept
{
    outputL = 0.0f;
    outputR = 0.0f;
    for (size_t i = 0; i < numTaps; ++i)
    {
        const float crossL = inputL + inputR * ((i & 1u) != 0u ? 0.18f : -0.12f);
        const float crossR = inputR + inputL * ((i & 1u) != 0u ? -0.12f : 0.18f);
        outputL += tapsL[i].process (crossL) * gains[i];
        outputR += tapsR[i].process (crossR) * gains[numTaps - 1u - i];
    }
}
