#include "Modulation.h"

#include <cmath>

void Modulation::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = juce::jmax (1.0, spec.sampleRate);

    for (int index = 0; index <= tableSize; ++index)
    {
        const float phase = static_cast<float> (index)
                            / static_cast<float> (tableSize);
        sineTable[static_cast<std::size_t> (index)] =
            std::sin (juce::MathConstants<float>::twoPi * phase);
    }

    constexpr std::array<float, lineCount> primaryRatesHz
    {
        0.031f, 0.037f, 0.043f, 0.049f,
        0.057f, 0.067f, 0.079f, 0.091f,
        0.107f, 0.127f, 0.149f, 0.173f,
        0.199f, 0.227f, 0.257f, 0.293f
    };

    constexpr std::array<float, lineCount> secondaryRatesHz
    {
        0.017f, 0.021f, 0.025f, 0.029f,
        0.034f, 0.039f, 0.045f, 0.052f,
        0.060f, 0.069f, 0.081f, 0.094f,
        0.109f, 0.126f, 0.146f, 0.169f
    };

    const float inverseSampleRate = 1.0f / static_cast<float> (sampleRate);
    for (std::size_t i = 0; i < phases.size(); ++i)
    {
        phaseIncrements[i] = primaryRatesHz[i] * inverseSampleRate;
        secondaryIncrements[i] = secondaryRatesHz[i] * inverseSampleRate;
    }

    globalDriftIncrement = 0.0067f * inverseSampleRate;

    // Approximately 2.4 seconds velocity decay.
    walkDamping = std::exp (-1.0f / (2.4f * static_cast<float> (sampleRate)));
    walkExcitation = 0.000035f * std::sqrt (48000.0f
                                           / static_cast<float> (sampleRate));

    reset();
}

void Modulation::reset() noexcept
{
    for (int line = 0; line < lineCount; ++line)
    {
        const auto i = static_cast<std::size_t> (line);
        const float goldenPhase = std::fmod (0.137f
                                             + static_cast<float> (line)
                                                   * 0.61803398875f,
                                             1.0f);
        phases[i] = goldenPhase;
        secondaryPhases[i] = std::fmod (goldenPhase * 1.73205080757f + 0.211f,
                                        1.0f);
        randomWalk[i] = 0.0f;
        randomVelocity[i] = 0.0f;
        lineOffsets[i] = 0.0f;
        noiseStates[i] = 0x9e3779b9u
                         ^ static_cast<std::uint32_t> ((line + 1) * 0x85ebca6bu);
    }

    globalDriftPhase = 0.193f;
}

void Modulation::setAmount (float percent) noexcept
{
    amount = juce::jlimit (0.0f, 1.0f, percent * 0.01f);
}

float Modulation::nextValue() noexcept
{
    globalDriftPhase += globalDriftIncrement;
    if (globalDriftPhase >= 1.0f)
        globalDriftPhase -= 1.0f;

    const float globalDrift = readSine (globalDriftPhase);

    float leftMean = 0.0f;
    float rightMean = 0.0f;

    for (int line = 0; line < lineCount; ++line)
    {
        const auto i = static_cast<std::size_t> (line);

        phases[i] += phaseIncrements[i];
        secondaryPhases[i] += secondaryIncrements[i];

        if (phases[i] >= 1.0f)
            phases[i] -= 1.0f;
        if (secondaryPhases[i] >= 1.0f)
            secondaryPhases[i] -= 1.0f;

        const float excitation = nextNoise (noiseStates[i]) * walkExcitation;
        randomVelocity[i] = randomVelocity[i] * walkDamping + excitation;
        randomWalk[i] = juce::jlimit (-0.32f, 0.32f,
                                      randomWalk[i] + randomVelocity[i]);

        // Soft restoring force prevents long-term drift to the bounds.
        randomVelocity[i] -= randomWalk[i] * 0.0000009f;

        const float primary = readSine (phases[i]);
        const float secondary = readSine (secondaryPhases[i]);
        const float quadrature = readSine (secondaryPhases[i] + 0.25f);
        const float alternatingDrift = ((line & 1) == 0 ? 1.0f : -1.0f)
                                       * globalDrift;

        const float blended = primary * 0.50f
                            + secondary * 0.23f
                            + quadrature * 0.10f
                            + randomWalk[i] * 0.14f
                            + alternatingDrift * 0.03f;

        lineOffsets[i] = juce::jlimit (-1.0f, 1.0f, blended) * amount;

        if ((line & 1) == 0)
            leftMean += lineOffsets[i];
        else
            rightMean += lineOffsets[i];
    }

    // Remove channel-common motion. This preserves decorrelation without
    // creating audible global pitch movement in the late field.
    leftMean *= 0.125f;
    rightMean *= 0.125f;

    for (int line = 0; line < lineCount; ++line)
    {
        const auto i = static_cast<std::size_t> (line);
        const float channelMean = ((line & 1) == 0) ? leftMean : rightMean;
        lineOffsets[i] = juce::jlimit (-1.0f, 1.0f,
                                      lineOffsets[i] - channelMean * 0.72f);
    }

    return 0.25f * (lineOffsets[0] + lineOffsets[5]
                    + lineOffsets[10] + lineOffsets[15]);
}

float Modulation::getOffset (int line) const noexcept
{
    if (line < 0 || line >= lineCount)
        return 0.0f;

    return lineOffsets[static_cast<std::size_t> (line)];
}

float Modulation::readSine (float phase) const noexcept
{
    phase -= std::floor (phase);

    const float tablePosition = phase * static_cast<float> (tableSize);
    const int index = static_cast<int> (tablePosition);
    const float fraction = tablePosition - static_cast<float> (index);

    const float a = sineTable[static_cast<std::size_t> (index & tableMask)];
    const float b = sineTable[static_cast<std::size_t> ((index + 1) & tableMask)];
    return a + (b - a) * fraction;
}

float Modulation::nextNoise (std::uint32_t& state) noexcept
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;

    const float unipolar = static_cast<float> (state & 0x00ffffffu)
                         / static_cast<float> (0x00ffffffu);
    return unipolar * 2.0f - 1.0f;
}
