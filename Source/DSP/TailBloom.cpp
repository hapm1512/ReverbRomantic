#include "TailBloom.h"

#include <algorithm>
#include <cmath>

void TailBloom::BloomDelay::prepare (int maximumDelaySamples)
{
    buffer.assign (static_cast<size_t> (juce::jmax (maximumDelaySamples + 1, 2)), 0.0f);
    writeIndex = 0;
}

void TailBloom::BloomDelay::reset() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
}

float TailBloom::BloomDelay::process (float input, int delaySamples, float feedback) noexcept
{
    if (buffer.empty())
        return input;

    const int size = static_cast<int> (buffer.size());
    const int clampedDelay = juce::jlimit (1, size - 1, delaySamples);
    int readIndex = writeIndex - clampedDelay;

    if (readIndex < 0)
        readIndex += size;

    const float delayed = buffer[static_cast<size_t> (readIndex)];
    const float output = delayed - feedback * input;
    buffer[static_cast<size_t> (writeIndex)] = input + feedback * output;

    if (++writeIndex >= size)
        writeIndex = 0;

    return output;
}

void TailBloom::prepare (const juce::dsp::ProcessSpec& spec)
{
    const auto sampleRate = static_cast<float> (juce::jmax (spec.sampleRate, 1.0));
    const auto msToSamples = [sampleRate] (float milliseconds)
    {
        return juce::jmax (1, static_cast<int> (std::round (milliseconds * 0.001f * sampleRate)));
    };

    leftDelaySamples  = { msToSamples (11.3f), msToSamples (17.9f) };
    rightDelaySamples = { msToSamples (13.7f), msToSamples (19.1f) };

    const int maximumDelay = *std::max_element (rightDelaySamples.begin(), rightDelaySamples.end()) + 8;

    for (auto& stage : leftStages)
        stage.prepare (maximumDelay);

    for (auto& stage : rightStages)
        stage.prepare (maximumDelay);

    attackCoeff = std::exp (-1.0f / (0.025f * sampleRate));
    releaseCoeff = std::exp (-1.0f / (0.900f * sampleRate));
    bloomSmoothCoeff = std::exp (-1.0f / (0.120f * sampleRate));

    reset();
}

void TailBloom::reset()
{
    envelope = 0.0f;
    adaptiveAmount = 0.0f;

    for (auto& stage : leftStages)
        stage.reset();

    for (auto& stage : rightStages)
        stage.reset();
}

void TailBloom::setAmount (float percent) noexcept
{
    amount = juce::jlimit (0.0f, 1.0f, percent * 0.01f);
}

void TailBloom::process (float& left, float& right) noexcept
{
    const float inputL = left;
    const float inputR = right;
    const float detector = juce::jmax (std::abs (inputL), std::abs (inputR));
    const float envelopeCoeff = detector > envelope ? attackCoeff : releaseCoeff;

    envelope = detector + envelopeCoeff * (envelope - detector);

    // Bloom remains restrained on strong early energy and increases smoothly in the late tail.
    const float lateTailWeight = 1.0f - juce::jlimit (0.0f, 1.0f, envelope * 2.4f);
    const float targetAmount = amount * lateTailWeight;
    adaptiveAmount = targetAmount + bloomSmoothCoeff * (adaptiveAmount - targetAmount);

    float bloomL = inputL;
    float bloomR = inputR;

    bloomL = leftStages[0].process (bloomL, leftDelaySamples[0], 0.48f);
    bloomR = rightStages[0].process (bloomR, rightDelaySamples[0], 0.46f);

    // Gentle cross-seeding prevents static stereo patterns without collapsing width.
    const float crossL = bloomL + bloomR * 0.085f;
    const float crossR = bloomR - bloomL * 0.085f;

    bloomL = leftStages[1].process (crossL, leftDelaySamples[1], 0.56f);
    bloomR = rightStages[1].process (crossR, rightDelaySamples[1], 0.54f);

    // Energy-aware wet contribution: density increases without changing T60.
    const float wet = adaptiveAmount * 0.32f;
    const float dryCompensation = 1.0f - wet * 0.18f;

    left  = inputL * dryCompensation + bloomL * wet;
    right = inputR * dryCompensation + bloomR * wet;

    left  = juce::jlimit (-4.0f, 4.0f, left);
    right = juce::jlimit (-4.0f, 4.0f, right);
}
