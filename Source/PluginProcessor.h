#pragma once

#include <JuceHeader.h>
#include <array>
#include <set>
#include <vector>
#include "DSP/HybridFDN16.h"
#include "DSP/ShimmerProcessor.h"
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

    // Epic 4D: A/B, favourites, user presets and undo/redo.
    void captureSnapshotA();
    void captureSnapshotB();
    void recallSnapshotA();
    void recallSnapshotB();
    bool isSnapshotBActive() const noexcept { return snapshotBActive.load(); }

    void setFactoryPresetFavourite (int index, bool shouldBeFavourite);
    bool isFactoryPresetFavourite (int index) const;
    juce::Array<int> getFavouriteFactoryPresets() const;

    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);
    bool renameUserPreset (const juce::String& oldName, const juce::String& newName);
    bool duplicateUserPreset (const juce::String& sourceName, const juce::String& destinationName);

    void pushUndoState();
    bool undoPresetChange();
    bool redoPresetChange();
    bool canUndoPresetChange() const noexcept { return ! undoHistory.empty(); }
    bool canRedoPresetChange() const noexcept { return ! redoHistory.empty(); }

    juce::AudioProcessorValueTreeState apvts;

    float getInputPeakLeft() const noexcept  { return inputPeakL.load(); }
    float getInputPeakRight() const noexcept { return inputPeakR.load(); }
    float getOutputPeakLeft() const noexcept { return outputPeakL.load(); }
    float getOutputPeakRight() const noexcept { return outputPeakR.load(); }
    float getSidechainPeak() const noexcept { return sidechainPeak.load(); }
    bool isSidechainConnected() const noexcept { return sidechainConnected.load(); }

private:
    static Algorithm sanitiseAlgorithm (float rawValue) noexcept;
    static void applyAlgorithmProfile (Algorithm algorithm,
                                       HybridFDN16::Parameters& parameters) noexcept;
    static const std::array<FactoryPreset, 48>& getFactoryPresetTable() noexcept;
    void setParameterPlainValue (const juce::String& id, float plainValue);

    juce::File getUserPresetDirectory() const;
    juce::File getUserPresetFile (const juce::String& name) const;
    juce::ValueTree createStateSnapshot();
    void restoreStateSnapshot (const juce::ValueTree& snapshot);
    void loadPersistentMetadata();
    void storePersistentMetadata();

    HybridFDN16 engine;
    ShimmerProcessor shimmer;

    juce::ValueTree snapshotA;
    juce::ValueTree snapshotB;
    std::atomic<bool> snapshotBActive { false };
    std::set<int> favouriteFactoryPresets;
    std::vector<juce::ValueTree> undoHistory;
    std::vector<juce::ValueTree> redoHistory;
    static constexpr size_t maxUndoStates = 32;

    std::atomic<int> currentFactoryPreset { 0 };
    std::atomic<float> inputPeakL  { 0.0f };
    std::atomic<float> inputPeakR  { 0.0f };
    std::atomic<float> outputPeakL { 0.0f };
    std::atomic<float> outputPeakR { 0.0f };
    std::atomic<float> sidechainPeak { 0.0f };
    std::atomic<bool> sidechainConnected { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbRomanticAudioProcessor)
};
