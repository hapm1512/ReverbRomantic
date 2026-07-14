#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/RomanticLookAndFeel.h"
class ReverbRomanticAudioProcessorEditor final: public juce::AudioProcessorEditor, private juce::Timer { public: explicit ReverbRomanticAudioProcessorEditor(ReverbRomanticAudioProcessor&); ~ReverbRomanticAudioProcessorEditor() override; void paint(juce::Graphics&) override; void resized() override; private: using SA=juce::AudioProcessorValueTreeState::SliderAttachment; using BA=juce::AudioProcessorValueTreeState::ButtonAttachment; void timerCallback() override{repaint();} void setup(juce::Slider&,const juce::String&); ReverbRomanticAudioProcessor& processor; RomanticLookAndFeel laf; juce::Label title,subtitle; std::array<juce::Slider,16> sliders; std::array<std::unique_ptr<SA>,16> attachments; juce::ToggleButton freeze{"FREEZE"},bypass{"BYPASS"}; std::unique_ptr<BA> freezeA,bypassA; JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbRomanticAudioProcessorEditor) };
