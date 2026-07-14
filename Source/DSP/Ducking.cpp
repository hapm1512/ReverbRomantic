#include "Ducking.h"
void Ducking::prepare (const juce::dsp::ProcessSpec& spec)
{
    attack = std::exp (-1.0f / (0.010f * static_cast<float> (spec.sampleRate)));
    release = std::exp (-1.0f / (0.300f * static_cast<float> (spec.sampleRate)));
    reset();
}
void Ducking::reset() { envelope = 0.0f; }
void Ducking::setAmount (float percent) noexcept { amount = juce::jlimit (0.0f, 1.0f, percent * 0.01f); }
void Ducking::process (float dryL, float dryR, float& wetL, float& wetR) noexcept
{
    const float detector = juce::jmax (std::abs (dryL), std::abs (dryR));
    const float coeff = detector > envelope ? attack : release;
    envelope = detector + coeff * (envelope - detector);
    const float gain = 1.0f - amount * juce::jlimit (0.0f, 1.0f, envelope * 2.5f);
    wetL *= gain;
    wetR *= gain;
}
