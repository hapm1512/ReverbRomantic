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
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String&) const;

    ReverbRomanticAudioProcessor& processor;
    RomanticLookAndFeel laf;

    juce::Label title, subtitle, categoryLabel, presetLabel, algorithmLabel, qualityLabel;
    juce::ComboBox categoryBox, presetBox, algorithmBox, qualityBox;
    juce::TextButton previousPreset { "<" }, nextPreset { ">" }, randomPreset { "RND" };
    juce::ToggleButton freeze { "FREEZE" }, bypass { "BYPASS" };

    std::vector<int> visiblePresetIndexes;

    std::array<juce::Slider, 16> sliders;
    std::array<juce::Label, 16> sliderLabels;
    std::array<std::unique_ptr<SA>, 16> sliderAttachments;
    std::unique_ptr<BA> freezeAttachment, bypassAttachment;
    std::unique_ptr<CA> algorithmAttachment, qualityAttachment;

    Meter inputMeter { "INPUT" };
    Meter outputMeter { "OUTPUT" };
    FFTGraph fftGraph;
    BottomBar bottomBar;

    std::array<juce::Rectangle<int>, 4> panelBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbRomanticAudioProcessorEditor)
};
