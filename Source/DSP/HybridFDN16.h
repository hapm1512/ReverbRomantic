#pragma once
#include <JuceHeader.h>
#include <array>
#include "DelayLine.h"
#include "Matrix16.h"
#include "Diffuser.h"
#include "EarlyReflection.h"
#include "Damping.h"
#include "Modulation.h"
#include "TailBloom.h"
#include "Ducking.h"
#include "StereoWidth.h"
#include "Limiter.h"

class HybridFDN16
{
public:
    struct Parameters
    {
        float decaySeconds = 4.2f;
        float preDelayMs = 38.0f;
        float sizePercent = 110.0f;
        float widthPercent = 125.0f;
        float diffusionPercent = 88.0f;
        float densityPercent = 94.0f;
        float modulationPercent = 22.0f;
        float bloomPercent = 45.0f;
        float duckingPercent = 18.0f;
        float lowCutHz = 120.0f;
        float highCutHz = 12000.0f;
        float warmthDb = 3.0f;
        float brightnessDb = -1.0f;
        int quality = 1;
        bool freeze = false;
    };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void setParameters (const Parameters& newParameters) noexcept;
    void processStereo (float inputL, float inputR, float& outputL, float& outputR) noexcept;

private:
    void updateFDNCoefficients() noexcept;
    float processHighPass (float input, float& inputState, float& outputState) noexcept;
    float processLowPass (float input, float& state) noexcept;

    std::array<RomanticDelayLine, 16> delays;
    std::array<RomanticDelayLine, 2> preDelay;
    Matrix16::Vector feedback {};
    Matrix16::Vector dampingState {};
    Matrix16::Vector baseDelaySamples {};
    Matrix16::Vector feedbackGains {};

    Diffuser diffuser;
    EarlyReflection earlyReflection;
    Damping outputDamping;
    Modulation modulationSource;
    TailBloom tailBloom;
    Ducking ducking;
    StereoWidth stereoWidth;
    Limiter limiter;

    // Prime delay lengths referenced to 48 kHz.
    static constexpr std::array<int, 16> primeDelaySamples48k {
        1493, 1801, 1987, 2111,
        2273, 2549, 2843, 2969,
        3253, 3413, 3547, 3821,
        4027, 4283, 4663, 4889
    };

    Parameters parameters;
    double sampleRate = 44100.0;
    float sizeScale = 1.0f;
    float densityScale = 0.95f;
    float lowPassCoefficient = 0.5f;
    float highPassCoefficient = 0.99f;
    float dampingCoefficient = 0.5f;
    float toneGain = 1.0f;
    float modulationDepthSamples = 0.0f;
    float earlyOutputGain = 0.32f;
    float lateOutputGain = 1.0f;
    float energyCompensation = 1.0f;
    float hpInputL = 0.0f, hpInputR = 0.0f;
    float hpOutputL = 0.0f, hpOutputR = 0.0f;
    float lpStateL = 0.0f, lpStateR = 0.0f;
};
