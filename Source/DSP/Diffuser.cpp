#include "Diffuser.h"

void Diffuser::AllPass::prepare (double sampleRate, float milliseconds)
{
    delay.prepare (sampleRate, 0.08);
    delay.setDelaySamples (milliseconds * 0.001f * static_cast<float> (sampleRate));
    reset();
}

void Diffuser::AllPass::reset()
{
    delay.reset();
    state = 0.0f;
}

float Diffuser::AllPass::process (float input) noexcept
{
    const float delayed = delay.process (input + gain * state);
    const float output = state - gain * delayed;
    state = delayed;
    return output;
}

void Diffuser::prepare (const juce::dsp::ProcessSpec& spec)
{
    constexpr std::array<float, 4> leftMs  { 3.1f, 4.7f, 7.3f, 11.9f };
    constexpr std::array<float, 4> rightMs { 3.7f, 5.3f, 8.9f, 13.1f };

    for (size_t i = 0; i < leftStages.size(); ++i)
    {
        leftStages[i].prepare (spec.sampleRate, leftMs[i]);
        rightStages[i].prepare (spec.sampleRate, rightMs[i]);
    }
}

void Diffuser::reset()
{
    for (auto& stage : leftStages)  stage.reset();
    for (auto& stage : rightStages) stage.reset();
}

void Diffuser::setAmount (float newAmount) noexcept
{
    amount = juce::jlimit (0.0f, 1.0f, newAmount);
    const float gain = juce::jmap (amount, 0.35f, 0.74f);
    for (auto& stage : leftStages)  stage.gain = gain;
    for (auto& stage : rightStages) stage.gain = gain;
}

void Diffuser::processStereo (float& left, float& right) noexcept
{
    const float dryL = left;
    const float dryR = right;

    for (auto& stage : leftStages)  left = stage.process (left);
    for (auto& stage : rightStages) right = stage.process (right);

    left  = juce::jmap (amount, dryL, left);
    right = juce::jmap (amount, dryR, right);
}
