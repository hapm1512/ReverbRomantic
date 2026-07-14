#pragma once

#include <JuceHeader.h>

class Damping
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;

    // Backward-compatible API used by HybridFDN16.
    void setCutoff (float cutoffHz) noexcept;

    // Prepared for the next integration step.
    void setDampingAmount (float amount) noexcept;
    void setAirLoss (float amount) noexcept;

    float processLeft (float input) noexcept;
    float processRight (float input) noexcept;

private:
    struct ChannelState
    {
        float lowPassA = 0.0f;
        float lowPassB = 0.0f;
        float air = 0.0f;
    };

    float process (float input, ChannelState& state) noexcept;
    void updateCoefficients() noexcept;

    double sampleRate = 44100.0;

    float requestedCutoffHz = 12000.0f;
    float dampingAmount = 0.35f;
    float airLossAmount = 0.20f;

    float coefficientA = 0.5f;
    float coefficientB = 0.35f;
    float airCoefficient = 0.15f;
    float outputCompensation = 1.0f;

    ChannelState leftState;
    ChannelState rightState;
};
