#pragma once
#include <JuceHeader.h>
#include "DSP/HybridFDN16.h"
#include "Parameters/ParameterLayout.h"
class ReverbRomanticAudioProcessor final: public juce::AudioProcessor { public: ReverbRomanticAudioProcessor(); ~ReverbRomanticAudioProcessor() override=default; void prepareToPlay(double,int) override; void releaseResources() override; bool isBusesLayoutSupported(const BusesLayout&) const override; void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override; juce::AudioProcessorEditor* createEditor() override; bool hasEditor() const override{return true;} const juce::String getName() const override{return JucePlugin_Name;} bool acceptsMidi() const override{return false;} bool producesMidi() const override{return false;} bool isMidiEffect() const override{return false;} double getTailLengthSeconds() const override; int getNumPrograms() override{return 1;} int getCurrentProgram() override{return 0;} void setCurrentProgram(int) override{} const juce::String getProgramName(int) override{return {};} void changeProgramName(int,const juce::String&) override{} void getStateInformation(juce::MemoryBlock&) override; void setStateInformation(const void*,int) override; juce::AudioProcessorValueTreeState apvts; float getInputPeakLeft() const noexcept { return inputPeakL.load(); }
float getInputPeakRight() const noexcept { return inputPeakR.load(); }
float getOutputPeakLeft() const noexcept { return outputPeakL.load(); }
float getOutputPeakRight() const noexcept { return outputPeakR.load(); } private: HybridFDN16 engine; std::atomic<float> inputPeakL { 0.0f }, inputPeakR { 0.0f };
std::atomic<float> outputPeakL { 0.0f }, outputPeakR { 0.0f }; JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbRomanticAudioProcessor) };
