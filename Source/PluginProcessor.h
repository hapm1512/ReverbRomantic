#pragma once

#include <JuceHeader.h>
#include <array>
#include "DSP/HybridFDN16.h"
#include "Parameters/ParameterLayout.h"

class ReverbRomanticAudioProcessor final : public juce::AudioProcessor
{
public:
    enum class Algorithm : int
    {
        romanticHall = 0,
        vocalPlate,
        studioRoom,
        chamber,
        cathedral,
        ambient
    };

    struct FactoryPreset
    {
        const char* name;
        const char* category;
        Algorithm algorithm;
        std::array<float, 16> values;
    };

    ReverbRomanticAudioProcessor();
    ~ReverbRomanticAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    int getNumFactoryPresets() const noexcept;
    juce::String getFactoryPresetName (int index) const;
    juce::String getFactoryPresetCategory (int index) const;
    juce::StringArray getFactoryPresetCategories() const;
    void loadFactoryPreset (int index);
    int getCurrentFactoryPreset() const noexcept { return currentFactoryPreset.load(); }

    juce::AudioProcessorValueTreeState apvts;

    float getInputPeakLeft() const noexcept  { return inputPeakL.load(); }
    float getInputPeakRight() const noexcept { return inputPeakR.load(); }
    float getOutputPeakLeft() const noexcept { return outputPeakL.load(); }
    float getOutputPeakRight() const noexcept { return outputPeakR.load(); }

private:
    static Algorithm sanitiseAlgorithm (float rawValue) noexcept;
    static void applyAlgorithmProfile (Algorithm algorithm,
                                       HybridFDN16::Parameters& parameters) noexcept;
    static const std::array<FactoryPreset, 48>& getFactoryPresetTable() noexcept;
    void setParameterPlainValue (const juce::String& id, float plainValue);

    HybridFDN16 engine;

    std::atomic<int> currentFactoryPreset { 0 };
    std::atomic<float> inputPeakL  { 0.0f };
    std::atomic<float> inputPeakR  { 0.0f };
    std::atomic<float> outputPeakL { 0.0f };
    std::atomic<float> outputPeakR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbRomanticAudioProcessor)
};
