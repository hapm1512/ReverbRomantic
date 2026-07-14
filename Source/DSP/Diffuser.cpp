#include "Diffuser.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float inverseSqrtTwo = 0.7071067811865475244f;

// Two banks with deliberately different, non-multiple delay times.
// Values are short enough to avoid audible echoes and long enough to build density.
constexpr std::array<float, 8> leftDelayMs
{
    2.17f, 3.11f, 4.73f, 6.37f,
    7.91f, 10.13f, 12.59f, 15.17f
};

constexpr std::array<float, 8> rightDelayMs
{
    2.71f, 3.83f, 5.29f, 6.97f,
    8.63f, 10.87f, 13.31f, 16.09f
};

// Slight gain variation prevents all stages from sharing the same decay signature.
constexpr std::array<float, 8> baseStageGains
{
    0.56f, 0.59f, 0.62f, 0.64f,
    0.66f, 0.68f, 0.70f, 0.72f
};
}

void Diffuser::AllPass::prepare (double sampleRate,
                                 float milliseconds,
                                 float stageGain)
{
    const auto safeRate = std::max (1.0, sampleRate);
    const auto delaySamples = static_cast<std::size_t> (std::max (
        1.0,
        std::round (safeRate * static_cast<double> (milliseconds) * 0.001)));

    buffer.assign (delaySamples, 0.0f);
    setGain (stageGain);
    reset();
}

void Diffuser::AllPass::reset()
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writePosition = 0;
}

void Diffuser::AllPass::setGain (float newGain) noexcept
{
    gain = juce::jlimit (0.0f, 0.79f, newGain);
}

float Diffuser::AllPass::process (float input) noexcept
{
    if (buffer.empty())
        return input;

    // Canonical Schroeder all-pass section.
    const float delayed = buffer[writePosition];
    const float internal = input + gain * delayed;
    const float output = delayed - gain * internal;

    buffer[writePosition] = internal;
    if (++writePosition >= buffer.size())
        writePosition = 0;

    return output;
}

void Diffuser::prepare (const juce::dsp::ProcessSpec& spec)
{
    for (std::size_t i = 0; i < stagesPerChannel; ++i)
    {
        leftStages[i].prepare (spec.sampleRate, leftDelayMs[i], baseStageGains[i]);
        rightStages[i].prepare (spec.sampleRate, rightDelayMs[i], baseStageGains[i]);
    }

    setAmount (amount);
    reset();
}

void Diffuser::reset()
{
    for (auto& stage : leftStages)
        stage.reset();

    for (auto& stage : rightStages)
        stage.reset();
}

void Diffuser::setAmount (float newAmount) noexcept
{
    amount = juce::jlimit (0.0f, 1.0f, newAmount);

    // Keep low settings open and transient-friendly.
    // High settings increase density without approaching instability.
    const float gainScale = juce::jmap (amount, 0.76f, 1.0f);

    for (std::size_t i = 0; i < stagesPerChannel; ++i)
    {
        const float stageGain = baseStageGains[i] * gainScale;
        leftStages[i].setGain (stageGain);
        rightStages[i].setGain (stageGain);
    }
}

void Diffuser::processStereo (float& left, float& right) noexcept
{
    const float dryLeft = left;
    const float dryRight = right;

    // Stage A: independent short all-pass chains create rapid local density.
    for (std::size_t i = 0; i < stageBoundary; ++i)
    {
        left = leftStages[i].process (left);
        right = rightStages[i].process (right);
    }

    // Orthogonal mid/side rotation decorrelates channels without energy loss.
    const float rotatedLeft = (left + right) * inverseSqrtTwo;
    const float rotatedRight = (left - right) * inverseSqrtTwo;
    left = rotatedLeft;
    right = rotatedRight;

    // Stage B: longer sections build a dense input cloud for the FDN.
    for (std::size_t i = stageBoundary; i < stagesPerChannel; ++i)
    {
        left = leftStages[i].process (left);
        right = rightStages[i].process (right);
    }

    // Inverse orthogonal rotation restores L/R orientation.
    const float wetLeft = (left + right) * inverseSqrtTwo;
    const float wetRight = (left - right) * inverseSqrtTwo;

    // Linear blend preserves the existing parameter response and unity endpoints.
    left = dryLeft + amount * (wetLeft - dryLeft);
    right = dryRight + amount * (wetRight - dryRight);
}
