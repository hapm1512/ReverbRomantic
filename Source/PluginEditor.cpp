#include "PluginEditor.h"
#include "GUI/Theme.h"

namespace
{
constexpr std::array<const char*, 16> parameterIds {
    Parameters::IDs::mix, Parameters::IDs::decay, Parameters::IDs::time, Parameters::IDs::preDelay,
    Parameters::IDs::size, Parameters::IDs::width, Parameters::IDs::warmth, Parameters::IDs::brightness,
    Parameters::IDs::diffusion, Parameters::IDs::density, Parameters::IDs::modulation, Parameters::IDs::bloom,
    Parameters::IDs::ducking, Parameters::IDs::lowCut, Parameters::IDs::highCut, Parameters::IDs::output
};
constexpr std::array<const char*, 16> names {
    "MIX", "DECAY", "TIME", "PRE-DELAY", "SIZE", "WIDTH", "WARMTH", "BRIGHTNESS",
    "DIFFUSION", "DENSITY", "MODULATION", "BLOOM", "DUCKING", "LOW CUT", "HIGH CUT", "OUTPUT"
};
constexpr std::array<const char*, 16> suffixes {
    " %", " s", " x", " ms", " %", " %", " dB", " dB",
    " %", " %", " %", " %", " %", " Hz", " Hz", " dB"
};
}

ReverbRomanticAudioProcessorEditor::ReverbRomanticAudioProcessorEditor (ReverbRomanticAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&laf);
    setResizable (true, true);
    setResizeLimits (860, 560, 1600, 980);
    setSize (1180, 720);

    title.setText ("REVERB ROMANTIC", juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centredLeft);
    title.setColour (juce::Label::textColourId, RomanticTheme::accentBright);
    title.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    subtitle.setText ("LYRICAL REVERB PRO", juce::dontSendNotification);
    subtitle.setJustificationType (juce::Justification::centredLeft);
    subtitle.setColour (juce::Label::textColourId, RomanticTheme::dim);
    subtitle.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    addAndMakeVisible (title);
    addAndMakeVisible (subtitle);

    presetLabel.setText ("PRESET", juce::dontSendNotification);
    qualityLabel.setText ("QUALITY", juce::dontSendNotification);
    for (auto* label : { &presetLabel, &qualityLabel })
    {
        label->setColour (juce::Label::textColourId, RomanticTheme::dim);
        label->setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        addAndMakeVisible (*label);
    }

    presetBox.addItemList ({ "Romantic Vocal", "Warm Ballad", "Wide Lyrical", "Long Dream", "Bright Hall" }, 1);
    presetBox.setSelectedId (1, juce::dontSendNotification);
    qualityBox.addItemList ({ "Eco", "Standard", "High", "Ultra" }, 1);
    addAndMakeVisible (presetBox);
    addAndMakeVisible (qualityBox);
    addAndMakeVisible (previousPreset);
    addAndMakeVisible (nextPreset);

    presetBox.onChange = [this] { applyPreset (presetBox.getSelectedItemIndex()); };
    previousPreset.onClick = [this]
    {
        const auto count = presetBox.getNumItems();
        presetBox.setSelectedItemIndex ((presetBox.getSelectedItemIndex() + count - 1) % count);
    };
    nextPreset.onClick = [this]
    {
        const auto count = presetBox.getNumItems();
        presetBox.setSelectedItemIndex ((presetBox.getSelectedItemIndex() + 1) % count);
    };

    for (size_t i = 0; i < sliders.size(); ++i)
    {
        setupSlider (sliders[i], sliderLabels[i], names[i], suffixes[i]);
        sliderAttachments[i] = std::make_unique<SA> (processor.apvts, parameterIds[i], sliders[i]);
    }

    addAndMakeVisible (freeze);
    addAndMakeVisible (bypass);
    freeze.setClickingTogglesState (true);
    bypass.setClickingTogglesState (true);
    freezeAttachment = std::make_unique<BA> (processor.apvts, Parameters::IDs::freeze, freeze);
    bypassAttachment = std::make_unique<BA> (processor.apvts, Parameters::IDs::bypass, bypass);
    qualityAttachment = std::make_unique<CA> (processor.apvts, Parameters::IDs::quality, qualityBox);

    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);
    addAndMakeVisible (fftGraph);
    addAndMakeVisible (bottomBar);

    startTimerHz (30);
}

ReverbRomanticAudioProcessorEditor::~ReverbRomanticAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ReverbRomanticAudioProcessorEditor::setupSlider (juce::Slider& slider, juce::Label& label,
                                                       const juce::String& name, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 19);
    slider.setTextValueSuffix (suffix);
    slider.setMouseDragSensitivity (240);
    slider.setDoubleClickReturnValue (true, 0.0);
    slider.setScrollWheelEnabled (true);
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, RomanticTheme::text);
    label.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
}

void ReverbRomanticAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient background (RomanticTheme::bgTop, 0.0f, 0.0f,
                                     RomanticTheme::bgBottom, 0.0f, (float) getHeight(), false);
    g.setGradientFill (background);
    g.fillAll();

    g.setColour (RomanticTheme::accent.withAlpha (0.05f));
    g.fillEllipse ((float) getWidth() * 0.56f, -150.0f, 480.0f, 360.0f);

    static constexpr std::array<const char*, 4> panelNames {
        "SPACE", "TONE", "TEXTURE", "CONTROL"
    };
    for (size_t i = 0; i < panelBounds.size(); ++i)
        drawPanel (g, panelBounds[i], panelNames[i]);
}

void ReverbRomanticAudioProcessorEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> area,
                                                     const juce::String& name) const
{
    auto b = area.toFloat();
    g.setColour (RomanticTheme::panel.withAlpha (0.84f));
    g.fillRoundedRectangle (b, 12.0f);
    g.setColour (RomanticTheme::border.withAlpha (0.78f));
    g.drawRoundedRectangle (b, 12.0f, 1.0f);
    auto titleArea = area.removeFromTop (28);
    g.setColour (RomanticTheme::accentBright.withAlpha (0.88f));
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawText (name, titleArea.reduced (12, 0), juce::Justification::centredLeft);
    g.setColour (RomanticTheme::border.withAlpha (0.45f));
    g.drawHorizontalLine (titleArea.getBottom(), (float) area.getX() + 10.0f, (float) area.getRight() - 10.0f);
}

void ReverbRomanticAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    bottomBar.setBounds (area.removeFromBottom (32));
    auto header = area.removeFromTop (88).reduced (22, 10);
    auto brand = header.removeFromLeft (juce::jmin (350, header.getWidth() / 3));
    title.setBounds (brand.removeFromTop (42));
    subtitle.setBounds (brand.removeFromTop (20));

    auto controls = header.reduced (4, 2);
    auto bypassArea = controls.removeFromRight (100);
    bypass.setBounds (bypassArea.reduced (4, 13));
    auto freezeArea = controls.removeFromRight (100);
    freeze.setBounds (freezeArea.reduced (4, 13));
    controls.removeFromRight (8);
    auto qualityArea = controls.removeFromRight (132);
    qualityLabel.setBounds (qualityArea.removeFromTop (17));
    qualityBox.setBounds (qualityArea.removeFromTop (34));
    controls.removeFromRight (8);
    auto presetArea = controls.removeFromRight (juce::jmin (300, controls.getWidth()));
    presetLabel.setBounds (presetArea.removeFromTop (17));
    previousPreset.setBounds (presetArea.removeFromLeft (34).reduced (1));
    nextPreset.setBounds (presetArea.removeFromRight (34).reduced (1));
    presetBox.setBounds (presetArea.reduced (4, 0));

    auto content = area.reduced (18, 10);
    auto meterStrip = content.removeFromRight (98);
    inputMeter.setBounds (meterStrip.removeFromLeft (46));
    meterStrip.removeFromLeft (4);
    outputMeter.setBounds (meterStrip.removeFromLeft (46));
    content.removeFromRight (12);

    const bool compact = content.getWidth() < 920;
    const int gap = 10;
    if (! compact)
    {
        const int panelWidth = (content.getWidth() - gap * 3) / 4;
        for (int i = 0; i < 4; ++i)
        {
            panelBounds[(size_t) i] = content.removeFromLeft (panelWidth);
            if (i < 3) content.removeFromLeft (gap);
        }
    }
    else
    {
        const int rowHeight = (content.getHeight() - gap) / 2;
        auto top = content.removeFromTop (rowHeight);
        content.removeFromTop (gap);
        auto bottom = content;
        const int width = (top.getWidth() - gap) / 2;
        panelBounds[0] = top.removeFromLeft (width);
        top.removeFromLeft (gap);
        panelBounds[1] = top;
        panelBounds[2] = bottom.removeFromLeft (width);
        bottom.removeFromLeft (gap);
        panelBounds[3] = bottom;
    }

    constexpr std::array<std::array<int, 4>, 4> panelSliderIndexes {{
        {{ 1, 2, 3, 4 }}, {{ 6, 7, 13, 14 }}, {{ 8, 9, 10, 11 }}, {{ 0, 5, 12, 15 }}
    }};

    for (size_t p = 0; p < panelBounds.size(); ++p)
    {
        auto inner = panelBounds[p].reduced (8);
        inner.removeFromTop (30);
        const int knobWidth = inner.getWidth() / 2;
        const int knobHeight = inner.getHeight() / 2;
        for (int slot = 0; slot < 4; ++slot)
        {
            const int row = slot / 2;
            const int col = slot % 2;
            auto cell = juce::Rectangle<int> (inner.getX() + col * knobWidth,
                                              inner.getY() + row * knobHeight,
                                              knobWidth, knobHeight).reduced (4);
            const auto index = (size_t) panelSliderIndexes[p][(size_t) slot];
            sliderLabels[index].setBounds (cell.removeFromTop (20));
            sliders[index].setBounds (cell);
        }
    }

    auto centre = getLocalBounds().reduced (170, 135);
    fftGraph.setBounds (centre);
    fftGraph.toBack();
}

void ReverbRomanticAudioProcessorEditor::timerCallback()
{
    inputMeter.setLevels (processor.getInputPeakLeft(), processor.getInputPeakRight());
    outputMeter.setLevels (processor.getOutputPeakLeft(), processor.getOutputPeakRight());
    fftGraph.setActivity (juce::jmax (processor.getOutputPeakLeft(), processor.getOutputPeakRight()));
}

void ReverbRomanticAudioProcessorEditor::setParameterValue (const juce::String& id, float plainValue)
{
    if (auto* parameter = processor.apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (plainValue));
}

void ReverbRomanticAudioProcessorEditor::applyPreset (int presetIndex)
{
    struct Preset { float mix, decay, time, preDelay, size, width, warmth, brightness, diffusion, density, modulation, bloom, ducking, lowCut, highCut, output; };
    static constexpr std::array<Preset, 5> presets {{
        { 35, 4.2f, 1.00f, 38, 110, 125, 3, -1, 88, 94, 22, 45, 18, 120, 12000, 0 },
        { 28, 3.4f, 0.92f, 24, 95, 115, 5, -2, 82, 90, 14, 34, 12, 140, 10500, 0 },
        { 42, 5.8f, 1.08f, 46, 135, 165, 2, 1, 92, 96, 28, 58, 22, 100, 14500, -1 },
        { 48, 8.6f, 1.24f, 68, 158, 180, 4, -3, 95, 98, 35, 76, 28, 90, 9800, -2 },
        { 32, 4.6f, 0.98f, 30, 120, 140, 0, 4, 86, 92, 18, 42, 16, 160, 16500, -1 }
    }};
    const auto& pr = presets[(size_t) juce::jlimit (0, (int) presets.size() - 1, presetIndex)];
    const std::array<float, 16> values { pr.mix, pr.decay, pr.time, pr.preDelay, pr.size, pr.width,
        pr.warmth, pr.brightness, pr.diffusion, pr.density, pr.modulation, pr.bloom,
        pr.ducking, pr.lowCut, pr.highCut, pr.output };
    for (size_t i = 0; i < values.size(); ++i)
        setParameterValue (parameterIds[i], values[i]);
}
