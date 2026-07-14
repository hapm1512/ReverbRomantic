#include "Modulation.h"

#include <cmath>

void Modulation::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = juce::jmax (1.0, spec.sampleRate);

    for (int index = 0; index <= tableSize; ++index)
    {
        const auto phase = static_cast<float> (index) / static_cast<float> (tableSize);
        sineTable[static_cast<std::size_t> (index)] =
            std::sin (juce::MathConstants<float>::twoPi * phase);
    }

    constexpr std::array<float, 4> ratesHz { 0.071f, 0.113f, 0.173f, 0.257f };
    for (std::size_t index = 0; index < ratesHz.size(); ++index)
        phaseIncrements[index] = ratesHz[index] / static_cast<float> (sampleRate);

    driftIncrement = 0.0073f / static_cast<float> (sampleRate);
    randomUpdateInterval = juce::jmax (1, static_cast<int> (sampleRate * 0.173));

    // Roughly 80 ms smoothing time for inaudible random movement.
    randomSmoothing = 1.0f - std::exp (-1.0f / static_cast<float> (sampleRate * 0.080));

    for (int line = 0; line < lineCount; ++line)
    {
        // Golden-ratio spacing avoids repeated phase relationships.
        phaseOffsets[static_cast<std::size_t> (line)] =
            std::fmod (static_cast<float> (line) * 0.61803398875f, 1.0f);
    }

    reset();
}

void Modulation::reset() noexcept
{
    phases = { 0.0f, 0.271f, 0.517f, 0.793f };
    driftPhase = 0.193f;
    lineOffsets.fill (0.0f);
    randomCurrent.fill (0.0f);
    randomTarget.fill (0.0f);
    randomUpdateCounter = 0;
    randomState = 0x6d2b79f5u;
    updateRandomTargets();
}

void Modulation::setAmount (float percent) noexcept
{
    amount = juce::jlimit (0.0f, 1.0f, percent * 0.01f);
}

float Modulation::nextValue() noexcept
{
    for (std::size_t index = 0; index < phases.size(); ++index)
    {
        phases[index] += phaseIncrements[index];
        if (phases[index] >= 1.0f)
            phases[index] -= 1.0f;
    }

    driftPhase += driftIncrement;
    if (driftPhase >= 1.0f)
        driftPhase -= 1.0f;

    if (++randomUpdateCounter >= randomUpdateInterval)
    {
        randomUpdateCounter = 0;
        updateRandomTargets();
    }

    const float slowDrift = readSine (driftPhase);

    for (int line = 0; line < lineCount; ++line)
    {
        const auto i = static_cast<std::size_t> (line);
        randomCurrent[i] += (randomTarget[i] - randomCurrent[i]) * randomSmoothing;

        const float offset = phaseOffsets[i];
        const float lfoA = readSine (phases[0] + offset);
        const float lfoB = readSine (phases[1] + offset * 1.731f);
        const float lfoC = readSine (phases[2] + offset * 2.173f);
        const float lfoD = readSine (phases[3] + offset * 2.879f);

        // Low-correlated blend. Random walk and drift remain intentionally subtle.
        const float blended = lfoA * 0.43f
                            + lfoB * 0.27f
                            + lfoC * 0.18f
                            + lfoD * 0.09f
                            + slowDrift * (0.035f + 0.002f * static_cast<float> (line))
                            + randomCurrent[i] * 0.12f;

        lineOffsets[i] = juce::jlimit (-1.0f, 1.0f, blended) * amount;
    }

    // Existing HybridFDN16 expects one global bipolar source.
    return (lineOffsets[0] + lineOffsets[5] + lineOffsets[10] + lineOffsets[15]) * 0.25f;
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

float Modulation::nextNoise() noexcept
{
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;

    const float unipolar = static_cast<float> (randomState & 0x00ffffffu)
                         / static_cast<float> (0x00ffffffu);
    return unipolar * 2.0f - 1.0f;
}

void Modulation::updateRandomTargets() noexcept
{
    for (auto& target : randomTarget)
        target = nextNoise() * 0.16f;

    // Vary the next update between approximately 120 and 260 ms.
    const float intervalSeconds = 0.12f + (nextNoise() * 0.5f + 0.5f) * 0.14f;
    randomUpdateInterval = juce::jmax (1, static_cast<int> (sampleRate * intervalSeconds));
}
