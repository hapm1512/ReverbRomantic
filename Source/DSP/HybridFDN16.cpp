#include "HybridFDN16.h"

#include <cmath>

HybridFDN16::RoomProfile HybridFDN16::getRoomProfile (RoomModel model) noexcept
{
    switch (model)
    {
        case RoomModel::room:
            return { 0.72f, 0.72f, 0.55f, -0.12f, -0.10f, 0.78f,
                     0.65f, -0.18f, 0.82f, 1.18f, 0.78f, 0.112f, 0.108f };

        case RoomModel::studio:
            return { 0.82f, 0.82f, 0.68f, -0.04f, -0.05f, 0.90f,
                     0.72f, -0.12f, 0.92f, 1.10f, 0.84f, 0.108f, 0.110f };

        case RoomModel::chamber:
            return { 0.96f, 0.96f, 0.84f, 0.04f, 0.02f, 0.94f,
                     0.90f, 0.02f, 1.02f, 1.02f, 0.96f, 0.105f, 0.114f };

        case RoomModel::cathedral:
            return { 1.34f, 1.30f, 1.28f, 0.10f, 0.08f, 0.72f,
                     1.12f, 0.14f, 1.12f, 0.76f, 1.18f, 0.096f, 0.120f };

        case RoomModel::plate:
            return { 0.88f, 1.04f, 0.34f, 0.14f, 0.10f, 1.12f,
                     1.18f, 0.08f, 1.08f, 0.62f, 1.14f, 0.102f, 0.118f };

        case RoomModel::hall:
        default:
            return { 1.12f, 1.12f, 1.00f, 0.07f, 0.05f, 0.84f,
                     1.00f, 0.08f, 1.08f, 0.88f, 1.08f, 0.100f, 0.117f };
    }
}

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

    sidechainHpfInput = 0.0f;
    sidechainHpfOutput = 0.0f;
    sidechainEnvelope = 0.0f;
    sidechainGain = 1.0f;
    sidechainGainReductionDb = 0.0f;
}

void HybridFDN16::updateFDNCoefficients() noexcept
{
    const float sampleRateScale = static_cast<float> (sampleRate / 48000.0);
    const float safeDecaySeconds = juce::jmax (0.2f, effectiveDecaySeconds);
    constexpr float minusLog1000 = -6.90775527898f;

    for (size_t i = 0; i < delays.size(); ++i)
    {
        const float delaySamples = static_cast<float> (primeDelaySamples48k[i])
                                   * sampleRateScale * sizeScale;

        baseDelaySamples[i] = delaySamples;
        delays[i].setDelaySamples (delaySamples);

        const float delaySeconds = delaySamples / static_cast<float> (sampleRate);
        feedbackGains[i] = std::exp (minusLog1000 * delaySeconds / safeDecaySeconds);
    }
}

void HybridFDN16::setParameters (const Parameters& newParameters) noexcept
{
    parameters = newParameters;
    parameters.decaySeconds = juce::jlimit (0.2f, 60.0f, parameters.decaySeconds);
    parameters.preDelayMs = juce::jlimit (0.0f, 250.0f, parameters.preDelayMs);
    parameters.sizePercent = juce::jlimit (25.0f, 200.0f, parameters.sizePercent);
    parameters.widthPercent = juce::jlimit (0.0f, 200.0f, parameters.widthPercent);
    parameters.diffusionPercent = juce::jlimit (0.0f, 100.0f, parameters.diffusionPercent);
    parameters.densityPercent = juce::jlimit (0.0f, 100.0f, parameters.densityPercent);
    parameters.modulationPercent = juce::jlimit (0.0f, 100.0f, parameters.modulationPercent);
    parameters.bloomPercent = juce::jlimit (0.0f, 100.0f, parameters.bloomPercent);
    parameters.duckingPercent = juce::jlimit (0.0f, 100.0f, parameters.duckingPercent);
    parameters.sidechainThresholdDb = juce::jlimit (-60.0f, 0.0f,
                                                     parameters.sidechainThresholdDb);
    parameters.sidechainAmountPercent = juce::jlimit (0.0f, 100.0f,
                                                       parameters.sidechainAmountPercent);
    parameters.sidechainAttackMs = juce::jlimit (0.1f, 200.0f,
                                                  parameters.sidechainAttackMs);
    parameters.sidechainReleaseMs = juce::jlimit (10.0f, 3000.0f,
                                                   parameters.sidechainReleaseMs);
    parameters.sidechainHighPassHz = juce::jlimit (20.0f, 1000.0f,
                                                    parameters.sidechainHighPassHz);
    parameters.lowCutHz = juce::jlimit (20.0f, 1000.0f, parameters.lowCutHz);
    parameters.highCutHz = juce::jlimit (1000.0f, 20000.0f, parameters.highCutHz);
    parameters.warmthDb = juce::jlimit (-12.0f, 12.0f, parameters.warmthDb);
    parameters.brightnessDb = juce::jlimit (-12.0f, 12.0f, parameters.brightnessDb);
    parameters.quality = juce::jlimit (0, 3, parameters.quality);

    roomProfile = getRoomProfile (parameters.roomModel);

    const float sizeNormalised = juce::jlimit (0.0f, 1.0f,
                                                (parameters.sizePercent - 25.0f) / 175.0f);
    const float densityNormalised = juce::jlimit (
        0.0f, 1.0f,
        parameters.densityPercent * 0.01f + roomProfile.densityBias);
    const float diffusionNormalised = juce::jlimit (
        0.0f, 1.0f,
        parameters.diffusionPercent * 0.01f + roomProfile.diffusionBias);

    sizeScale = juce::jlimit (0.25f, 2.35f,
                              parameters.sizePercent * 0.01f * roomProfile.delayScale);
    effectiveDecaySeconds = juce::jlimit (
        0.2f, 60.0f, parameters.decaySeconds * roomProfile.decayScale);
    densityScale = juce::jmap (densityNormalised, 0.72f, 1.0f);
    effectiveDiffusionMix = diffusionNormalised;
    injectionGain = roomProfile.injectionGain;
    tapGain = roomProfile.tapGain;

    const float effectivePreDelayMs = juce::jlimit (
        0.0f, 250.0f, parameters.preDelayMs * roomProfile.preDelayScale);
    for (auto& delay : preDelay)
        delay.setDelaySamples (effectivePreDelayMs * 0.001f
                               * static_cast<float> (sampleRate));

    updateFDNCoefficients();

    diffuser.setAmount (diffusionNormalised);
    earlyReflection.setSize (sizeScale);
    earlyReflection.setShape (static_cast<EarlyReflection::Shape> (parameters.roomModel));

    const float modelHighCut = juce::jlimit (
        1000.0f, 20000.0f, parameters.highCutHz * roomProfile.dampingScale);
    outputDamping.setCutoff (modelHighCut);
    earlyReflection.setHighCut (modelHighCut);

    modulationSource.setAmount (juce::jlimit (
        0.0f, 100.0f, parameters.modulationPercent * roomProfile.modulationScale));
    tailBloom.setAmount (juce::jlimit (
        0.0f, 100.0f, parameters.bloomPercent + roomProfile.bloomBias * 100.0f));
    ducking.setAmount (parameters.duckingPercent);
    stereoWidth.setWidth (juce::jlimit (
        0.0f, 200.0f, parameters.widthPercent * roomProfile.widthScale));

    const float brightnessRatio = std::pow (2.0f, parameters.brightnessDb / 12.0f);
    const float effectiveHighCut = juce::jlimit (
        1000.0f, 20000.0f, modelHighCut * brightnessRatio);

    lowPassCoefficient = std::exp (-juce::MathConstants<float>::twoPi
                                   * effectiveHighCut
                                   / static_cast<float> (sampleRate));
    dampingCoefficient = std::exp (-juce::MathConstants<float>::twoPi
                                   * modelHighCut
                                   / static_cast<float> (sampleRate));
    highPassCoefficient = std::exp (-juce::MathConstants<float>::twoPi
                                    * parameters.lowCutHz
                                    / static_cast<float> (sampleRate));

    toneGain = juce::Decibels::decibelsToGain (parameters.warmthDb * 0.12f);

    const float qualityScale = std::array<float, 4> { 0.55f, 0.8f, 1.0f, 1.15f }
        [static_cast<size_t> (parameters.quality)];
    // Sub-sample movement breaks static modes without audible chorus.
    const float maxDepthMs = 0.075f * qualityScale * roomProfile.modulationScale;
    modulationDepthSamples = maxDepthMs * 0.001f
                             * static_cast<float> (sampleRate);

    earlyOutputGain = juce::jmap (sizeNormalised, 0.42f, 0.20f)
                      * roomProfile.earlyScale;
    const float lateSizeGain = juce::jmap (sizeNormalised, 0.84f, 1.10f)
                               * roomProfile.lateScale;
    const float densityNormalisation = juce::jmap (densityNormalised, 1.04f, 0.88f);
    const float diffusionNormalisation = juce::jmap (diffusionNormalised, 1.0f, 0.92f);
    lateOutputGain = densityNormalisation * diffusionNormalisation * lateSizeGain;

    const float densityEnergy = juce::jmap (densityNormalised, 1.08f, 0.92f);
    const float sizeEnergy = juce::jmap (sizeNormalised, 1.04f, 0.94f);
    const float modelEnergy = 1.0f / juce::jmax (
        0.65f, 0.48f * roomProfile.earlyScale + 0.52f * roomProfile.lateScale);
    energyCompensation = densityEnergy * sizeEnergy * modelEnergy;

    const float sampleRateFloat = static_cast<float> (sampleRate);
    sidechainHpfCoefficient = std::exp (
        -juce::MathConstants<float>::twoPi
        * parameters.sidechainHighPassHz / sampleRateFloat);

    sidechainAttackCoefficient = std::exp (
        -1.0f / (0.001f * parameters.sidechainAttackMs * sampleRateFloat));
    sidechainReleaseCoefficient = std::exp (
        -1.0f / (0.001f * parameters.sidechainReleaseMs * sampleRateFloat));

    sidechainThresholdLinear = juce::Decibels::decibelsToGain (
        parameters.sidechainThresholdDb);

    // Maximum attenuation is 24 dB at 100% amount.
    sidechainMinimumGain = juce::Decibels::decibelsToGain (
        -24.0f * parameters.sidechainAmountPercent * 0.01f);
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

float HybridFDN16::processSidechainDetector (float sidechainL,
                                               float sidechainR) noexcept
{
    if (! parameters.sidechainEnabled
        || parameters.sidechainAmountPercent <= 0.0f)
    {
        sidechainGain += (1.0f - sidechainGain) * 0.02f;
        sidechainGainReductionDb = juce::Decibels::gainToDecibels (
            juce::jmax (sidechainGain, 0.000001f));
        return sidechainGain;
    }

    const float mono = 0.5f * (sidechainL + sidechainR);

    // One-pole high-pass removes low-frequency pumping from the detector.
    const float filtered = sidechainHpfCoefficient
                           * (sidechainHpfOutput + mono - sidechainHpfInput);
    sidechainHpfInput = mono;
    sidechainHpfOutput = filtered;

    const float detectorInput = std::abs (filtered);
    const float envelopeCoefficient = detectorInput > sidechainEnvelope
                                        ? sidechainAttackCoefficient
                                        : sidechainReleaseCoefficient;
    sidechainEnvelope = detectorInput
                        + envelopeCoefficient * (sidechainEnvelope - detectorInput);

    // 6 dB soft knee around the threshold.
    constexpr float kneeRatio = 1.9952623f;
    const float kneeStart = sidechainThresholdLinear / kneeRatio;
    const float kneeEnd = sidechainThresholdLinear * kneeRatio;

    float activity = 0.0f;
    if (sidechainEnvelope >= kneeEnd)
    {
        activity = 1.0f;
    }
    else if (sidechainEnvelope > kneeStart)
    {
        const float normalised = (sidechainEnvelope - kneeStart)
                                 / juce::jmax (0.000001f, kneeEnd - kneeStart);
        activity = normalised * normalised * (3.0f - 2.0f * normalised);
    }

    const float targetGain = juce::jmap (activity, 1.0f, sidechainMinimumGain);
    const float gainCoefficient = targetGain < sidechainGain
                                    ? sidechainAttackCoefficient
                                    : sidechainReleaseCoefficient;
    sidechainGain = targetGain
                    + gainCoefficient * (sidechainGain - targetGain);

    sidechainGainReductionDb = juce::Decibels::gainToDecibels (
        juce::jmax (sidechainGain, 0.000001f));
    return sidechainGain;
}

void HybridFDN16::processStereo (float inputL,
                                 float inputR,
                                 float& outputL,
                                 float& outputR) noexcept
{
    processStereo (inputL, inputR, 0.0f, 0.0f, outputL, outputR);
}

void HybridFDN16::processStereo (float inputL,
                                 float inputR,
                                 float sidechainL,
                                 float sidechainR,
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
    Matrix16::orthogonal (mixedFeedback);

    float lateL = 0.0f;
    float lateR = 0.0f;
    modulationSource.nextValue();

    for (int i = 0; i < 16; ++i)
    {
        const auto index = static_cast<size_t> (i);
        const float modulationSamples = modulationSource.getOffset (i)
                                        * modulationDepthSamples;
        delays[index].setModulationOffset (modulationSamples);

        const float source = (i & 1) != 0 ? diffusedR : diffusedL;
        const float injection = parameters.freeze ? 0.0f : source * injectionGain;
        const float loopGain = parameters.freeze
                                 ? 0.99995f
                                 : feedbackGains[index] * densityScale;

        const float delayed = delays[index].process (
            injection + mixedFeedback[index] * loopGain);

        if (parameters.freeze)
        {
            feedback[index] = delayed;
        }
        else
        {
            dampingState[index] = delayed * (1.0f - dampingCoefficient)
                                  + dampingState[index] * dampingCoefficient;
            feedback[index] = juce::jmap (
                effectiveDiffusionMix, delayed, dampingState[index]);
        }

        const float tap = feedback[index] * tapGain;
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

    tailBloom.process (outputL, outputR);

    if (parameters.processOutputStage)
        stereoWidth.process (outputL, outputR);

    // External sidechain affects only the wet reverb signal.
    const float externalDuckGain = processSidechainDetector (sidechainL, sidechainR);
    outputL *= externalDuckGain;
    outputR *= externalDuckGain;

    // Existing internal ducking remains available for legacy callers.
    ducking.process (inputL, inputR, outputL, outputR);

    if (parameters.processOutputStage)
        limiter.process (outputL, outputR);
}
