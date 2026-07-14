#include "PluginProcessor.h"
#include "PluginEditor.h"
ReverbRomanticAudioProcessor::ReverbRomanticAudioProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),apvts(*this,nullptr,"PARAMETERS",Parameters::createParameterLayout()){}
void ReverbRomanticAudioProcessor::prepareToPlay(double sr,int bs){engine.prepare({sr,(juce::uint32)bs,2});}
void ReverbRomanticAudioProcessor::releaseResources(){engine.reset();}
bool ReverbRomanticAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {auto o=l.getMainOutputChannelSet();return (o==juce::AudioChannelSet::mono()||o==juce::AudioChannelSet::stereo())&&o==l.getMainInputChannelSet();}
double ReverbRomanticAudioProcessor::getTailLengthSeconds() const {return apvts.getRawParameterValue(Parameters::IDs::decay)->load();}
void ReverbRomanticAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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

    engine.setParameters (
        apvts.getRawParameterValue (Parameters::IDs::decay)->load()
            * apvts.getRawParameterValue (Parameters::IDs::time)->load(),
        apvts.getRawParameterValue (Parameters::IDs::size)->load(),
        apvts.getRawParameterValue (Parameters::IDs::highCut)->load(),
        apvts.getRawParameterValue (Parameters::IDs::diffusion)->load(),
        apvts.getRawParameterValue (Parameters::IDs::density)->load(),
        apvts.getRawParameterValue (Parameters::IDs::modulation)->load(),
        apvts.getRawParameterValue (Parameters::IDs::freeze)->load() > 0.5f);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float dryL = left[sample];
        const float dryR = right[sample];
        float wetL = 0.0f, wetR = 0.0f;
        engine.processStereo (dryL, dryR, wetL, wetR);
        left[sample] = (dryL * (1.0f - mix) + wetL * mix) * outputGain;
        right[sample] = (dryR * (1.0f - mix) + wetR * mix) * outputGain;
    }

    outputPeakL.store (buffer.getMagnitude (0, 0, buffer.getNumSamples()));
    outputPeakR.store (buffer.getMagnitude (rightChannel, 0, buffer.getNumSamples()));
}
juce::AudioProcessorEditor* ReverbRomanticAudioProcessor::createEditor(){return new ReverbRomanticAudioProcessorEditor(*this);}void ReverbRomanticAudioProcessor::getStateInformation(juce::MemoryBlock& m){auto x=apvts.copyState().createXml();copyXmlToBinary(*x,m);}void ReverbRomanticAudioProcessor::setStateInformation(const void* d,int s){auto x=getXmlFromBinary(d,s);if(x&&x->hasTagName(apvts.state.getType()))apvts.replaceState(juce::ValueTree::fromXml(*x));}juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new ReverbRomanticAudioProcessor();}
