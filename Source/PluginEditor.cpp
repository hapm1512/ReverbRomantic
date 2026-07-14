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

    categoryLabel.setText ("CATEGORY", juce::dontSendNotification);
    presetLabel.setText ("PRESET", juce::dontSendNotification);
    algorithmLabel.setText ("ALGORITHM", juce::dontSendNotification);
    qualityLabel.setText ("QUALITY", juce::dontSendNotification);
    for (auto* label : { &categoryLabel, &presetLabel, &algorithmLabel, &qualityLabel })
    {
        label->setColour (juce::Label::textColourId, RomanticTheme::dim);
        label->setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        addAndMakeVisible (*label);
    }

    categoryBox.addItemList (processor.getFactoryPresetCategories(), 1);
    categoryBox.setSelectedId (1, juce::dontSendNotification);
    algorithmBox.addItemList ({ "Romantic Hall", "Vocal Plate", "Studio Room",
                                "Chamber", "Cathedral", "Ambient" }, 1);
    qualityBox.addItemList ({ "Eco", "Standard", "High", "Ultra" }, 1);
    addAndMakeVisible (categoryBox);
    addAndMakeVisible (presetBox);
    addAndMakeVisible (algorithmBox);
    addAndMakeVisible (qualityBox);
    addAndMakeVisible (previousPreset);
    addAndMakeVisible (nextPreset);
    addAndMakeVisible (randomPreset);

    categoryBox.onChange = [this] { rebuildPresetList(); };
    presetBox.onChange = [this]
    {
        if (presetBox.getSelectedItemIndex() >= 0)
            loadVisiblePreset (presetBox.getSelectedItemIndex());
    };
    previousPreset.onClick = [this]
    {
        const auto count = presetBox.getNumItems();
        if (count > 0)
            presetBox.setSelectedItemIndex ((presetBox.getSelectedItemIndex() + count - 1) % count);
    };
    nextPreset.onClick = [this]
    {
        const auto count = presetBox.getNumItems();
        if (count > 0)
            presetBox.setSelectedItemIndex ((presetBox.getSelectedItemIndex() + 1) % count);
    };
    randomPreset.onClick = [this]
    {
        const auto count = presetBox.getNumItems();
        if (count > 0)
            presetBox.setSelectedItemIndex (juce::Random::getSystemRandom().nextInt (count));
    };

    rebuildPresetList();
    selectProcessorPreset();

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
    algorithmAttachment = std::make_unique<CA> (processor.apvts, Parameters::IDs::algorithm, algorithmBox);
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
    auto qualityArea = controls.removeFromRight (108);
    qualityLabel.setBounds (qualityArea.removeFromTop (17));
    qualityBox.setBounds (qualityArea.removeFromTop (34));
    controls.removeFromRight (7);
    auto algorithmArea = controls.removeFromRight (juce::jmin (150, controls.getWidth() / 3));
    algorithmLabel.setBounds (algorithmArea.removeFromTop (17));
    algorithmBox.setBounds (algorithmArea.removeFromTop (34));
    controls.removeFromRight (7);
    auto presetArea = controls.removeFromRight (juce::jmin (235, controls.getWidth() / 2));
    presetLabel.setBounds (presetArea.removeFromTop (17));
    previousPreset.setBounds (presetArea.removeFromLeft (30).reduced (1));
    randomPreset.setBounds (presetArea.removeFromRight (42).reduced (1));
    nextPreset.setBounds (presetArea.removeFromRight (30).reduced (1));
    presetBox.setBounds (presetArea.reduced (3, 0));
    controls.removeFromRight (7);
    auto categoryArea = controls.removeFromRight (juce::jmin (130, controls.getWidth()));
    categoryLabel.setBounds (categoryArea.removeFromTop (17));
    categoryBox.setBounds (categoryArea.removeFromTop (34));

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

void ReverbRomanticAudioProcessorEditor::rebuildPresetList()
{
    const auto selectedCategory = categoryBox.getText();
    const int currentPreset = processor.getCurrentFactoryPreset();

    visiblePresetIndexes.clear();
    presetBox.clear (juce::dontSendNotification);

    for (int index = 0; index < processor.getNumFactoryPresets(); ++index)
    {
        if (selectedCategory == "All"
            || processor.getFactoryPresetCategory (index) == selectedCategory)
        {
            visiblePresetIndexes.push_back (index);
            presetBox.addItem (processor.getFactoryPresetName (index),
                               static_cast<int> (visiblePresetIndexes.size()));
        }
    }

    auto found = std::find (visiblePresetIndexes.begin(), visiblePresetIndexes.end(), currentPreset);
    const int selectedIndex = found != visiblePresetIndexes.end()
                                ? static_cast<int> (std::distance (visiblePresetIndexes.begin(), found))
                                : 0;
    if (! visiblePresetIndexes.empty())
        presetBox.setSelectedItemIndex (selectedIndex, juce::dontSendNotification);
}

void ReverbRomanticAudioProcessorEditor::loadVisiblePreset (int visibleIndex)
{
    if (juce::isPositiveAndBelow (visibleIndex, static_cast<int> (visiblePresetIndexes.size())))
        processor.loadFactoryPreset (visiblePresetIndexes[static_cast<size_t> (visibleIndex)]);
}

void ReverbRomanticAudioProcessorEditor::selectProcessorPreset()
{
    const int processorIndex = processor.getCurrentFactoryPreset();
    const auto found = std::find (visiblePresetIndexes.begin(), visiblePresetIndexes.end(), processorIndex);
    if (found != visiblePresetIndexes.end())
        presetBox.setSelectedItemIndex (static_cast<int> (std::distance (visiblePresetIndexes.begin(), found)),
                                        juce::dontSendNotification);
}
