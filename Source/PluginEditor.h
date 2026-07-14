#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <vector>
#include "PluginProcessor.h"
#include "GUI/RomanticLookAndFeel.h"
#include "GUI/Meter.h"
#include "GUI/FFTGraph.h"
#include "GUI/BottomBar.h"

class ReverbRomanticAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit ReverbRomanticAudioProcessorEditor (ReverbRomanticAudioProcessor&);
    ~ReverbRomanticAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void timerCallback() override;
    void setupSlider (juce::Slider&, juce::Label&, const juce::String&, const juce::String&);
    void rebuildPresetList();
    void loadVisiblePreset (int visibleIndex);
    void selectProcessorPreset();
    void refreshUserPresetList();
    void updateFavouriteButton();
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String&) const;
    void setupSidechainSlider (juce::Slider&, juce::Label&,
                               const juce::String&, const juce::String&);

    ReverbRomanticAudioProcessor& processor;
    RomanticLookAndFeel laf;

    juce::Label title, subtitle, categoryLabel, presetLabel, algorithmLabel, qualityLabel;
    juce::ComboBox categoryBox, presetBox, algorithmBox, qualityBox;
    juce::TextButton previousPreset { "<" }, nextPreset { ">" }, randomPreset { "RND" };
    juce::ToggleButton freeze { "FREEZE" }, bypass { "BYPASS" };
    std::array<juce::Slider, 3> freezeSliders;
    std::array<juce::Label, 3> freezeLabels;
    juce::ToggleButton sidechainEnable { "SIDECHAIN" };
    juce::Label sidechainStatus;

    juce::ToggleButton shimmerEnable { "SHIMMER" };
    juce::ComboBox shimmerPitchBox;
    juce::Label shimmerPitchLabel;
    std::array<juce::Slider, 3> shimmerSliders;
    std::array<juce::Label, 3> shimmerLabels;

    juce::ComboBox userPresetBox;
    juce::TextButton favouriteButton { "FAV" };
    juce::TextButton saveUserButton { "SAVE" }, loadUserButton { "LOAD" };
    juce::TextButton renameUserButton { "REN" }, duplicateUserButton { "DUP" };
    juce::TextButton deleteUserButton { "DEL" };
    juce::TextButton snapshotAButton { "A" }, snapshotBButton { "B" };
    juce::TextButton captureAButton { "SET A" }, captureBButton { "SET B" };
    juce::TextButton undoButton { "UNDO" }, redoButton { "REDO" };
    juce::String selectedUserPreset;

    std::vector<int> visiblePresetIndexes;

    std::array<juce::Slider, 16> sliders;
    std::array<juce::Slider, 5> sidechainSliders;
    std::array<juce::Label, 5> sidechainLabels;
    std::array<juce::Label, 16> sliderLabels;
    std::array<std::unique_ptr<SA>, 16> sliderAttachments;
    std::array<std::unique_ptr<SA>, 5> sidechainSliderAttachments;
    std::unique_ptr<BA> freezeAttachment, bypassAttachment, sidechainEnableAttachment;
    std::array<std::unique_ptr<SA>, 3> freezeSliderAttachments;
    std::unique_ptr<BA> shimmerEnableAttachment;
    std::array<std::unique_ptr<SA>, 3> shimmerSliderAttachments;
    std::unique_ptr<CA> shimmerPitchAttachment;
    std::unique_ptr<CA> algorithmAttachment, qualityAttachment;

    Meter inputMeter { "INPUT" };
    Meter outputMeter { "OUTPUT" };
    FFTGraph fftGraph;
    BottomBar bottomBar;

    std::array<juce::Rectangle<int>, 4> panelBounds;
    juce::Rectangle<int> shimmerPanelBounds;
    juce::Rectangle<int> freezePanelBounds;
    juce::Rectangle<int> sidechainPanelBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbRomanticAudioProcessorEditor)
};
