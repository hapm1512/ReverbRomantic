#include "PluginProcessor.h"
#include "PluginEditor.h"

ReverbRomanticAudioProcessor::ReverbRomanticAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", Parameters::createParameterLayout())
{
}

void ReverbRomanticAudioProcessor::prepareToPlay (double sampleRate,
                                                   int samplesPerBlock)
{
    engine.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
}

void ReverbRomanticAudioProcessor::releaseResources()
{
    engine.reset();
}

bool ReverbRomanticAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono()
            || output == juce::AudioChannelSet::stereo())
           && output == layouts.getMainInputChannelSet();
}

double ReverbRomanticAudioProcessor::getTailLengthSeconds() const
{
    const float decay = apvts.getRawParameterValue (Parameters::IDs::decay)->load();
    const float time = apvts.getRawParameterValue (Parameters::IDs::time)->load();
    const auto algorithm = sanitiseAlgorithm (
        apvts.getRawParameterValue (Parameters::IDs::algorithm)->load());

    float algorithmScale = 1.0f;
    switch (algorithm)
    {
        case Algorithm::romanticHall: algorithmScale = 1.12f; break;
        case Algorithm::vocalPlate:   algorithmScale = 1.04f; break;
        case Algorithm::studioRoom:   algorithmScale = 0.82f; break;
        case Algorithm::chamber:      algorithmScale = 0.96f; break;
        case Algorithm::cathedral:    algorithmScale = 1.30f; break;
        case Algorithm::ambient:      algorithmScale = 1.55f; break;
    }

    return static_cast<double> (decay * time * algorithmScale);
}

ReverbRomanticAudioProcessor::Algorithm
ReverbRomanticAudioProcessor::sanitiseAlgorithm (float rawValue) noexcept
{
    const int index = juce::jlimit (0, 5, static_cast<int> (rawValue));
    return static_cast<Algorithm> (index);
}

void ReverbRomanticAudioProcessor::applyAlgorithmProfile (
    Algorithm algorithm,
    HybridFDN16::Parameters& p) noexcept
{
    // Profiles shape the DSP internally without overwriting user automation.
    // Every visible control remains effective around the selected algorithm.
    switch (algorithm)
    {
        case Algorithm::romanticHall:
            p.roomModel = HybridFDN16::RoomModel::hall;
            p.decaySeconds *= 1.12f;
            p.preDelayMs *= 1.05f;
            p.sizePercent *= 1.08f;
            p.widthPercent *= 1.06f;
            p.diffusionPercent += 4.0f;
            p.densityPercent += 3.0f;
            p.modulationPercent += 4.0f;
            p.bloomPercent += 8.0f;
            p.warmthDb += 0.8f;
            break;

        case Algorithm::vocalPlate:
            p.roomModel = HybridFDN16::RoomModel::plate;
            p.decaySeconds *= 0.94f;
            p.preDelayMs *= 0.72f;
            p.sizePercent *= 0.88f;
            p.widthPercent *= 1.08f;
            p.diffusionPercent += 8.0f;
            p.densityPercent += 5.0f;
            p.modulationPercent += 3.0f;
            p.bloomPercent += 2.0f;
            p.lowCutHz += 35.0f;
            p.brightnessDb += 1.2f;
            break;

        case Algorithm::studioRoom:
            p.roomModel = HybridFDN16::RoomModel::studio;
            p.decaySeconds *= 0.72f;
            p.preDelayMs *= 0.45f;
            p.sizePercent *= 0.72f;
            p.widthPercent *= 0.88f;
            p.diffusionPercent -= 7.0f;
            p.densityPercent -= 9.0f;
            p.modulationPercent *= 0.55f;
            p.bloomPercent *= 0.45f;
            p.highCutHz *= 0.92f;
            break;

        case Algorithm::chamber:
            p.roomModel = HybridFDN16::RoomModel::chamber;
            p.decaySeconds *= 0.92f;
            p.preDelayMs *= 0.82f;
            p.sizePercent *= 0.92f;
            p.widthPercent *= 0.96f;
            p.diffusionPercent += 1.0f;
            p.densityPercent += 1.0f;
            p.modulationPercent *= 0.85f;
            p.bloomPercent *= 0.82f;
            p.warmthDb += 0.4f;
            break;

        case Algorithm::cathedral:
            p.roomModel = HybridFDN16::RoomModel::cathedral;
            p.decaySeconds *= 1.30f;
            p.preDelayMs *= 1.28f;
            p.sizePercent *= 1.20f;
            p.widthPercent *= 1.10f;
            p.diffusionPercent += 5.0f;
            p.densityPercent += 4.0f;
            p.modulationPercent += 3.0f;
            p.bloomPercent += 12.0f;
            p.highCutHz *= 0.86f;
            p.warmthDb += 0.5f;
            break;

        case Algorithm::ambient:
            p.roomModel = HybridFDN16::RoomModel::cathedral;
            p.decaySeconds *= 1.55f;
            p.preDelayMs *= 1.18f;
            p.sizePercent *= 1.26f;
            p.widthPercent *= 1.18f;
            p.diffusionPercent += 8.0f;
            p.densityPercent += 6.0f;
            p.modulationPercent += 10.0f;
            p.bloomPercent += 22.0f;
            p.duckingPercent *= 0.65f;
            p.highCutHz *= 0.92f;
            p.brightnessDb += 0.6f;
            break;
    }

    // Keep profile offsets inside the ranges accepted by HybridFDN16.
    p.decaySeconds      = juce::jlimit (0.2f, 60.0f, p.decaySeconds);
    p.preDelayMs        = juce::jlimit (0.0f, 250.0f, p.preDelayMs);
    p.sizePercent       = juce::jlimit (25.0f, 200.0f, p.sizePercent);
    p.widthPercent      = juce::jlimit (0.0f, 200.0f, p.widthPercent);
    p.diffusionPercent  = juce::jlimit (0.0f, 100.0f, p.diffusionPercent);
    p.densityPercent    = juce::jlimit (0.0f, 100.0f, p.densityPercent);
    p.modulationPercent = juce::jlimit (0.0f, 100.0f, p.modulationPercent);
    p.bloomPercent      = juce::jlimit (0.0f, 100.0f, p.bloomPercent);
    p.duckingPercent    = juce::jlimit (0.0f, 100.0f, p.duckingPercent);
    p.lowCutHz          = juce::jlimit (20.0f, 1000.0f, p.lowCutHz);
    p.highCutHz         = juce::jlimit (1000.0f, 20000.0f, p.highCutHz);
    p.warmthDb          = juce::jlimit (-12.0f, 12.0f, p.warmthDb);
    p.brightnessDb      = juce::jlimit (-12.0f, 12.0f, p.brightnessDb);
}

void ReverbRomanticAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    const auto rightChannel = juce::jmin (1, buffer.getNumChannels() - 1);
    inputPeakL.store (buffer.getMagnitude (0, 0, buffer.getNumSamples()));
    inputPeakR.store (buffer.getMagnitude (rightChannel, 0, buffer.getNumSamples()));

    if (apvts.getRawParameterValue (Parameters::IDs::bypass)->load() > 0.5f)
    {
        outputPeakL.store (inputPeakL.load());
        outputPeakR.store (inputPeakR.load());
        return;
    }

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

    const float mix = apvts.getRawParameterValue (Parameters::IDs::mix)->load() * 0.01f;
    const float outputGain = juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (Parameters::IDs::output)->load());

    HybridFDN16::Parameters dspParameters;
    dspParameters.decaySeconds =
        apvts.getRawParameterValue (Parameters::IDs::decay)->load()
        * apvts.getRawParameterValue (Parameters::IDs::time)->load();
    dspParameters.preDelayMs = apvts.getRawParameterValue (Parameters::IDs::preDelay)->load();
    dspParameters.sizePercent = apvts.getRawParameterValue (Parameters::IDs::size)->load();
    dspParameters.widthPercent = apvts.getRawParameterValue (Parameters::IDs::width)->load();
    dspParameters.diffusionPercent = apvts.getRawParameterValue (Parameters::IDs::diffusion)->load();
    dspParameters.densityPercent = apvts.getRawParameterValue (Parameters::IDs::density)->load();
    dspParameters.modulationPercent = apvts.getRawParameterValue (Parameters::IDs::modulation)->load();
    dspParameters.bloomPercent = apvts.getRawParameterValue (Parameters::IDs::bloom)->load();
    dspParameters.duckingPercent = apvts.getRawParameterValue (Parameters::IDs::ducking)->load();
    dspParameters.lowCutHz = apvts.getRawParameterValue (Parameters::IDs::lowCut)->load();
    dspParameters.highCutHz = apvts.getRawParameterValue (Parameters::IDs::highCut)->load();
    dspParameters.warmthDb = apvts.getRawParameterValue (Parameters::IDs::warmth)->load();
    dspParameters.brightnessDb = apvts.getRawParameterValue (Parameters::IDs::brightness)->load();
    dspParameters.quality = static_cast<int> (
        apvts.getRawParameterValue (Parameters::IDs::quality)->load());
    dspParameters.freeze =
        apvts.getRawParameterValue (Parameters::IDs::freeze)->load() > 0.5f;

    const auto algorithm = sanitiseAlgorithm (
        apvts.getRawParameterValue (Parameters::IDs::algorithm)->load());
    applyAlgorithmProfile (algorithm, dspParameters);
    engine.setParameters (dspParameters);

    const bool isMono = buffer.getNumChannels() == 1;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float dryL = left[sample];
        const float dryR = isMono ? dryL : right[sample];
        float wetL = 0.0f;
        float wetR = 0.0f;

        engine.processStereo (dryL, dryR, wetL, wetR);

        if (isMono)
        {
            const float wetMono = 0.5f * (wetL + wetR);
            left[sample] = (dryL * (1.0f - mix) + wetMono * mix) * outputGain;
        }
        else
        {
            left[sample] = (dryL * (1.0f - mix) + wetL * mix) * outputGain;
            right[sample] = (dryR * (1.0f - mix) + wetR * mix) * outputGain;
        }
    }

    outputPeakL.store (buffer.getMagnitude (0, 0, buffer.getNumSamples()));
    outputPeakR.store (buffer.getMagnitude (rightChannel, 0, buffer.getNumSamples()));
}

juce::AudioProcessorEditor* ReverbRomanticAudioProcessor::createEditor()
{
    return new ReverbRomanticAudioProcessorEditor (*this);
}

void ReverbRomanticAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destination);
}

void ReverbRomanticAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverbRomanticAudioProcessor();
}
