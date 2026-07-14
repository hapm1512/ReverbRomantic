#include "HybridFDN16.h"

void HybridFDN16::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    for (int i = 0; i < 16; ++i)
    {
        delays[static_cast<size_t> (i)].prepare (sampleRate, 0.5);
        phases[static_cast<size_t> (i)] = static_cast<float> (i) / 16.0f;
        rates[static_cast<size_t> (i)] = 0.07f + 0.011f * static_cast<float> (i);
    }

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
    for (auto& delay : delays) delay.reset();
    for (auto& delay : preDelay) delay.reset();

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

    sizeScale = juce::jlimit (0.25f, 2.0f, parameters.sizePercent * 0.01f);
    densityScale = juce::jmap (juce::jlimit (0.0f, 1.0f, parameters.densityPercent * 0.01f), 0.72f, 1.0f);

    for (auto& delay : preDelay)
        delay.setDelaySamples (parameters.preDelayMs * 0.001f * static_cast<float> (sampleRate));

    diffuser.setAmount (parameters.diffusionPercent * 0.01f);
    earlyReflection.setSize (sizeScale);
    outputDamping.setCutoff (parameters.highCutHz);
    modulationSource.setAmount (parameters.modulationPercent);
    tailBloom.setAmount (parameters.bloomPercent);
    ducking.setAmount (parameters.duckingPercent);
    stereoWidth.setWidth (parameters.widthPercent);

    const float brightnessRatio = std::pow (2.0f, parameters.brightnessDb / 12.0f);
    const float effectiveHighCut = juce::jlimit (1000.0f, 20000.0f, parameters.highCutHz * brightnessRatio);
    lowPassCoefficient = std::exp (-juce::MathConstants<float>::twoPi
                                   * effectiveHighCut / static_cast<float> (sampleRate));
    toneGain = juce::Decibels::decibelsToGain (parameters.warmthDb * 0.12f);
    modulationQualityScale = std::array<float, 4> { 0.55f, 0.8f, 1.0f, 1.15f }[static_cast<size_t> (parameters.quality)];
    highPassCoefficient = std::exp (-juce::MathConstants<float>::twoPi
                                    * parameters.lowCutHz / static_cast<float> (sampleRate));
}

float HybridFDN16::processHighPass (float input, float& inputState, float& outputState) noexcept
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

void HybridFDN16::processStereo (float inputL, float inputR, float& outputL, float& outputR) noexcept
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
    Matrix16::hadamard (mixedFeedback);

    float lateL = 0.0f;
    float lateR = 0.0f;
    const float globalModulation = modulationSource.nextValue();
    const float dampingCoefficient = std::exp (-juce::MathConstants<float>::twoPi
                                               * parameters.highCutHz / static_cast<float> (sampleRate));

    for (int i = 0; i < 16; ++i)
    {
        const auto index = static_cast<size_t> (i);
        phases[index] += rates[index] / static_cast<float> (sampleRate);
        if (phases[index] >= 1.0f)
            phases[index] -= 1.0f;

        const float localLfo = std::sin (juce::MathConstants<float>::twoPi * phases[index]);
        const float modulationMs = (localLfo * 0.75f + globalModulation * 0.25f)
                                   * parameters.modulationPercent * 0.008f * modulationQualityScale;
        const float delayMs = baseMs[index] * sizeScale + modulationMs;
        delays[index].setDelaySamples (delayMs * 0.001f * static_cast<float> (sampleRate));

        const float feedbackGain = parameters.freeze
            ? 0.9995f
            : std::pow (0.001f, (baseMs[index] * sizeScale * 0.001f) / parameters.decaySeconds);

        const float source = (i & 1) != 0 ? diffusedR : diffusedL;
        const float injection = parameters.freeze ? 0.0f : source * 0.105f;
        const float delayed = delays[index].process (injection + mixedFeedback[index] * feedbackGain * densityScale);

        dampingState[index] = delayed * (1.0f - dampingCoefficient)
                              + dampingState[index] * dampingCoefficient;
        const float diffusionMix = juce::jlimit (0.0f, 1.0f, parameters.diffusionPercent * 0.01f);
        feedback[index] = juce::jmap (diffusionMix, delayed, dampingState[index]);

        const float tap = feedback[index] * 0.115f;
        lateL += ((i % 4) < 2 ? 1.0f : -1.0f) * tap;
        lateR += (((i + 1) % 4) < 2 ? 1.0f : -1.0f) * tap;
    }

    outputL = earlyL * 0.32f + lateL;
    outputR = earlyR * 0.32f + lateR;

    outputL = processHighPass (outputL * toneGain, hpInputL, hpOutputL);
    outputR = processHighPass (outputR * toneGain, hpInputR, hpOutputR);
    outputL = processLowPass (outputL, lpStateL);
    outputR = processLowPass (outputR, lpStateR);

    tailBloom.process (outputL, outputR);
    stereoWidth.process (outputL, outputR);
    ducking.process (inputL, inputR, outputL, outputR);
    limiter.process (outputL, outputR);
}
