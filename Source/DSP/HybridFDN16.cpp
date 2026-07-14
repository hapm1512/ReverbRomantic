#include "HybridFDN16.h"

#include <cmath>

void HybridFDN16::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = juce::jmax (1.0, spec.sampleRate);

    for (auto& delay : delays)
        delay.prepare (sampleRate, 0.5);

    for (auto& delay : preDelay)
        delay.prepare (sampleRate, 0.3);

    diffuser.prepare (spec);
    earlyReflection.prepare (spec);
    outputDamping.prepare (spec);
    modulationSource.prepare (spec);
    tailBloom.prepare (spec);
    ducking.prepare (spec);
    stereoWidth.prepare (spec);
    limiter.prepare (spec);

    setParameters (parameters);
    reset();
}

void HybridFDN16::reset()
{
    for (auto& delay : delays)
        delay.reset();

    for (auto& delay : preDelay)
        delay.reset();

    feedback.fill (0.0f);
    dampingState.fill (0.0f);
    diffuser.reset();
    earlyReflection.reset();
    outputDamping.reset();
    modulationSource.reset();
    tailBloom.reset();
    ducking.reset();
    stereoWidth.reset();
    limiter.reset();

    hpInputL = hpInputR = 0.0f;
    hpOutputL = hpOutputR = 0.0f;
    lpStateL = lpStateR = 0.0f;
}

void HybridFDN16::updateFDNCoefficients() noexcept
{
    const float sampleRateScale = static_cast<float> (sampleRate / 48000.0);
    const float safeDecaySeconds = juce::jmax (0.2f, parameters.decaySeconds);
    constexpr float minusLog1000 = -6.90775527898f;

    for (size_t i = 0; i < delays.size(); ++i)
    {
        const float delaySamples = static_cast<float> (primeDelaySamples48k[i])
                                   * sampleRateScale * sizeScale;

        baseDelaySamples[i] = delaySamples;
        delays[i].setDelaySamples (delaySamples);

        // T60: amplitude reaches -60 dB after decaySeconds.
        const float delaySeconds = delaySamples / static_cast<float> (sampleRate);
        feedbackGains[i] = std::exp (minusLog1000 * delaySeconds / safeDecaySeconds);
    }
}

void HybridFDN16::setParameters (const Parameters& newParameters) noexcept
{
    parameters = newParameters;
    parameters.decaySeconds = juce::jlimit (0.2f, 60.0f, parameters.decaySeconds);
    parameters.preDelayMs = juce::jlimit (0.0f, 250.0f, parameters.preDelayMs);
    parameters.lowCutHz = juce::jlimit (20.0f, 1000.0f, parameters.lowCutHz);
    parameters.highCutHz = juce::jlimit (1000.0f, 20000.0f, parameters.highCutHz);
    parameters.warmthDb = juce::jlimit (-12.0f, 12.0f, parameters.warmthDb);
    parameters.brightnessDb = juce::jlimit (-12.0f, 12.0f, parameters.brightnessDb);
    parameters.quality = juce::jlimit (0, 3, parameters.quality);

    const float sizeNormalised = juce::jlimit (0.0f, 1.0f,
                                                (parameters.sizePercent - 25.0f) / 175.0f);
    const float densityNormalised = juce::jlimit (0.0f, 1.0f,
                                                  parameters.densityPercent * 0.01f);
    const float diffusionNormalised = juce::jlimit (0.0f, 1.0f,
                                                     parameters.diffusionPercent * 0.01f);

    sizeScale = juce::jlimit (0.25f, 2.0f, parameters.sizePercent * 0.01f);
    densityScale = juce::jmap (densityNormalised, 0.72f, 1.0f);

    for (auto& delay : preDelay)
        delay.setDelaySamples (parameters.preDelayMs * 0.001f
                               * static_cast<float> (sampleRate));

    updateFDNCoefficients();

    diffuser.setAmount (diffusionNormalised);
    earlyReflection.setSize (sizeScale);
    outputDamping.setCutoff (parameters.highCutHz);
    modulationSource.setAmount (parameters.modulationPercent);
    tailBloom.setAmount (parameters.bloomPercent);
    ducking.setAmount (parameters.duckingPercent);
    stereoWidth.setWidth (parameters.widthPercent);

    const float brightnessRatio = std::pow (2.0f, parameters.brightnessDb / 12.0f);
    const float effectiveHighCut = juce::jlimit (1000.0f, 20000.0f,
                                                 parameters.highCutHz * brightnessRatio);

    lowPassCoefficient = std::exp (-juce::MathConstants<float>::twoPi
                                   * effectiveHighCut
                                   / static_cast<float> (sampleRate));
    dampingCoefficient = std::exp (-juce::MathConstants<float>::twoPi
                                   * parameters.highCutHz
                                   / static_cast<float> (sampleRate));
    highPassCoefficient = std::exp (-juce::MathConstants<float>::twoPi
                                    * parameters.lowCutHz
                                    / static_cast<float> (sampleRate));

    toneGain = juce::Decibels::decibelsToGain (parameters.warmthDb * 0.12f);

    const float qualityScale = std::array<float, 4> { 0.55f, 0.8f, 1.0f, 1.15f }
        [static_cast<size_t> (parameters.quality)];
    const float maxDepthMs = 0.34f * qualityScale;
    modulationDepthSamples = maxDepthMs * 0.001f
                             * static_cast<float> (sampleRate);

    // Small spaces favour early reflections; large spaces favour the late field.
    earlyOutputGain = juce::jmap (sizeNormalised, 0.42f, 0.20f);
    const float lateSizeGain = juce::jmap (sizeNormalised, 0.84f, 1.10f);

    const float densityNormalisation = juce::jmap (densityNormalised, 1.04f, 0.88f);
    const float diffusionNormalisation = juce::jmap (diffusionNormalised, 1.0f, 0.92f);
    lateOutputGain = densityNormalisation * diffusionNormalisation * lateSizeGain;

    // Precomputed wet-energy compensation avoids gain jumps during parameter moves.
    const float densityEnergy = juce::jmap (densityNormalised, 1.08f, 0.92f);
    const float sizeEnergy = juce::jmap (sizeNormalised, 1.04f, 0.94f);
    energyCompensation = densityEnergy * sizeEnergy;
}

float HybridFDN16::processHighPass (float input,
                                    float& inputState,
                                    float& outputState) noexcept
{
    const float output = highPassCoefficient * (outputState + input - inputState);
    inputState = input;
    outputState = output;
    return output;
}

float HybridFDN16::processLowPass (float input, float& state) noexcept
{
    state = input * (1.0f - lowPassCoefficient) + state * lowPassCoefficient;
    return state;
}

void HybridFDN16::processStereo (float inputL,
                                 float inputR,
                                 float& outputL,
                                 float& outputR) noexcept
{
    const float predelayedL = preDelay[0].process (inputL);
    const float predelayedR = preDelay[1].process (inputR);

    float diffusedL = predelayedL;
    float diffusedR = predelayedR;
    diffuser.processStereo (diffusedL, diffusedR);

    float earlyL = 0.0f;
    float earlyR = 0.0f;
    earlyReflection.processStereo (predelayedL, predelayedR, earlyL, earlyR);

    auto mixedFeedback = feedback;
    Matrix16::householder (mixedFeedback);

    float lateL = 0.0f;
    float lateR = 0.0f;
    modulationSource.nextValue();

    const float diffusionMix = juce::jlimit (0.0f, 1.0f,
                                              parameters.diffusionPercent * 0.01f);

    for (int i = 0; i < 16; ++i)
    {
        const auto index = static_cast<size_t> (i);
        const float modulationSamples = modulationSource.getOffset (i)
                                        * modulationDepthSamples;
        delays[index].setModulationOffset (modulationSamples);

        const float source = (i & 1) != 0 ? diffusedR : diffusedL;
        const float injection = parameters.freeze ? 0.0f : source * 0.105f;

        // Freeze bypasses all loop attenuation, including density scaling.
        const float loopGain = parameters.freeze
                                 ? 1.0f
                                 : feedbackGains[index] * densityScale;

        const float delayed = delays[index].process (
            injection + mixedFeedback[index] * loopGain);

        if (parameters.freeze)
        {
            // Preserve the existing field without HF loss during infinite sustain.
            feedback[index] = delayed;
        }
        else
        {
            dampingState[index] = delayed * (1.0f - dampingCoefficient)
                                  + dampingState[index] * dampingCoefficient;
            feedback[index] = juce::jmap (diffusionMix, delayed, dampingState[index]);
        }

        const float tap = feedback[index] * 0.115f;
        lateL += ((i % 4) < 2 ? 1.0f : -1.0f) * tap;
        lateR += (((i + 1) % 4) < 2 ? 1.0f : -1.0f) * tap;
    }

    outputL = (earlyL * earlyOutputGain + lateL * lateOutputGain)
              * energyCompensation;
    outputR = (earlyR * earlyOutputGain + lateR * lateOutputGain)
              * energyCompensation;

    outputL = processHighPass (outputL * toneGain, hpInputL, hpOutputL);
    outputR = processHighPass (outputR * toneGain, hpInputR, hpOutputR);
    outputL = processLowPass (outputL, lpStateL);
    outputR = processLowPass (outputR, lpStateR);

    // Late enhancement precedes spatial widening and dynamics processing.
    tailBloom.process (outputL, outputR);
    stereoWidth.process (outputL, outputR);
    ducking.process (inputL, inputR, outputL, outputR);
    limiter.process (outputL, outputR);
}
