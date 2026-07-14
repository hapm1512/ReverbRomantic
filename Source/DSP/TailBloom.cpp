#include "TailBloom.h"
void TailBloom::prepare (const juce::dsp::ProcessSpec& spec)
{
    attack = std::exp (-1.0f / (0.080f * static_cast<float> (spec.sampleRate)));
    release = std::exp (-1.0f / (0.650f * static_cast<float> (spec.sampleRate)));
    reset();
}
void TailBloom::reset() { envelope = 0.0f; }
void TailBloom::setAmount (float percent) noexcept { amount = juce::jlimit (0.0f, 1.0f, percent * 0.01f); }
void TailBloom::process (float& left, float& right) noexcept
{
    const float detector = juce::jmax (std::abs (left), std::abs (right));
    const float coeff = detector > envelope ? attack : release;
    envelope = detector + coeff * (envelope - detector);
    const float bloomGain = 1.0f + amount * juce::jlimit (0.0f, 1.0f, 1.0f - envelope * 2.0f) * 0.35f;
    left *= bloomGain;
    right *= bloomGain;
}
