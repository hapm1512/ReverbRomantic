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

constexpr std::array<const char*, 3> shimmerParameterIds {
    Parameters::IDs::shimmerMix,
    Parameters::IDs::shimmerFeedback,
    Parameters::IDs::shimmerTone
};
constexpr std::array<const char*, 3> shimmerNames {
    "MIX", "FEEDBACK", "TONE"
};
constexpr std::array<const char*, 3> shimmerSuffixes {
    " %", " %", " Hz"
};

constexpr std::array<const char*, 3> freezeParameterIds {
    Parameters::IDs::freezeMix,
    Parameters::IDs::freezeFade,
    Parameters::IDs::freezeDamp
};
constexpr std::array<const char*, 3> freezeNames {
    "MIX", "FADE", "DAMP"
};
constexpr std::array<const char*, 3> freezeSuffixes {
    " %", " ms", " %"
};

constexpr std::array<const char*, 5> sidechainParameterIds {
    Parameters::IDs::sidechainThreshold,
    Parameters::IDs::sidechainAmount,
    Parameters::IDs::sidechainAttack,
    Parameters::IDs::sidechainRelease,
    Parameters::IDs::sidechainHPF
};
constexpr std::array<const char*, 5> sidechainNames {
    "THRESHOLD", "AMOUNT", "ATTACK", "RELEASE", "DETECT HPF"
};
constexpr std::array<const char*, 5> sidechainSuffixes {
    " dB", " %", " ms", " ms", " Hz"
};
}

ReverbRomanticAudioProcessorEditor::ReverbRomanticAudioProcessorEditor (ReverbRomanticAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&laf);
    setResizable (true, true);
    setResizeLimits (900, 860, 1600, 1280);
    setSize (1180, 940);

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

    for (auto* component : { static_cast<juce::Component*> (&userPresetBox),
                             static_cast<juce::Component*> (&favouriteButton),
                             static_cast<juce::Component*> (&saveUserButton),
                             static_cast<juce::Component*> (&loadUserButton),
                             static_cast<juce::Component*> (&renameUserButton),
                             static_cast<juce::Component*> (&duplicateUserButton),
                             static_cast<juce::Component*> (&deleteUserButton),
                             static_cast<juce::Component*> (&snapshotAButton),
                             static_cast<juce::Component*> (&snapshotBButton),
                             static_cast<juce::Component*> (&captureAButton),
                             static_cast<juce::Component*> (&captureBButton),
                             static_cast<juce::Component*> (&undoButton),
                             static_cast<juce::Component*> (&redoButton) })
        addAndMakeVisible (*component);

    userPresetBox.setEditableText (true);
    userPresetBox.setTextWhenNothingSelected ("User preset name");
    userPresetBox.onChange = [this]
    {
        if (userPresetBox.getSelectedItemIndex() >= 0)
            selectedUserPreset = userPresetBox.getText();
    };

    favouriteButton.onClick = [this]
    {
        const int index = processor.getCurrentFactoryPreset();
        if (index >= 0)
        {
            processor.setFactoryPresetFavourite (
                index, ! processor.isFactoryPresetFavourite (index));
            updateFavouriteButton();
        }
    };

    saveUserButton.onClick = [this]
    {
        auto name = userPresetBox.getText().trim();
        if (name.isEmpty() || name == "User preset name")
            name = "User Preset " + juce::String (processor.getUserPresetNames().size() + 1);
        if (processor.saveUserPreset (name))
        {
            selectedUserPreset = name;
            refreshUserPresetList();
        }
    };
    loadUserButton.onClick = [this]
    {
        const auto name = userPresetBox.getText().trim();
        if (processor.loadUserPreset (name))
            selectedUserPreset = name;
    };
    deleteUserButton.onClick = [this]
    {
        const auto name = selectedUserPreset.isNotEmpty() ? selectedUserPreset
                                                         : userPresetBox.getText().trim();
        if (processor.deleteUserPreset (name))
        {
            selectedUserPreset.clear();
            refreshUserPresetList();
        }
    };
    renameUserButton.onClick = [this]
    {
        const auto newName = userPresetBox.getText().trim();
        if (selectedUserPreset.isNotEmpty() && newName.isNotEmpty()
            && processor.renameUserPreset (selectedUserPreset, newName))
        {
            selectedUserPreset = newName;
            refreshUserPresetList();
        }
    };
    duplicateUserButton.onClick = [this]
    {
        const auto source = selectedUserPreset.isNotEmpty() ? selectedUserPreset
                                                            : userPresetBox.getText().trim();
        if (source.isNotEmpty())
        {
            auto destination = source + " Copy";
            int suffix = 2;
            while (processor.getUserPresetNames().contains (destination))
                destination = source + " Copy " + juce::String (suffix++);
            if (processor.duplicateUserPreset (source, destination))
            {
                selectedUserPreset = destination;
                refreshUserPresetList();
            }
        }
    };

    captureAButton.onClick = [this] { processor.captureSnapshotA(); };
    captureBButton.onClick = [this] { processor.captureSnapshotB(); };
    snapshotAButton.onClick = [this] { processor.recallSnapshotA(); };
    snapshotBButton.onClick = [this] { processor.recallSnapshotB(); };
    undoButton.onClick = [this] { processor.undoPresetChange(); };
    redoButton.onClick = [this] { processor.redoPresetChange(); };

    rebuildPresetList();
    selectProcessorPreset();
    refreshUserPresetList();
    updateFavouriteButton();

    for (size_t i = 0; i < sliders.size(); ++i)
    {
        setupSlider (sliders[i], sliderLabels[i], names[i], suffixes[i]);
        sliderAttachments[i] = std::make_unique<SA> (processor.apvts, parameterIds[i], sliders[i]);
    }

    for (size_t i = 0; i < shimmerSliders.size(); ++i)
    {
        setupSidechainSlider (shimmerSliders[i], shimmerLabels[i],
                              shimmerNames[i], shimmerSuffixes[i]);
        shimmerSliderAttachments[i] = std::make_unique<SA> (
            processor.apvts, shimmerParameterIds[i], shimmerSliders[i]);
    }

    shimmerEnable.setClickingTogglesState (true);
    shimmerEnableAttachment = std::make_unique<BA> (
        processor.apvts, Parameters::IDs::shimmerEnable, shimmerEnable);
    addAndMakeVisible (shimmerEnable);

    shimmerPitchLabel.setText ("PITCH", juce::dontSendNotification);
    shimmerPitchLabel.setJustificationType (juce::Justification::centred);
    shimmerPitchLabel.setColour (juce::Label::textColourId, RomanticTheme::text);
    shimmerPitchLabel.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    addAndMakeVisible (shimmerPitchLabel);

    shimmerPitchBox.addItemList ({ "+7 semitones", "+12 semitones" }, 1);
    shimmerPitchAttachment = std::make_unique<CA> (
        processor.apvts, Parameters::IDs::shimmerPitch, shimmerPitchBox);
    addAndMakeVisible (shimmerPitchBox);

    for (size_t i = 0; i < freezeSliders.size(); ++i)
    {
        setupSidechainSlider (freezeSliders[i], freezeLabels[i],
                              freezeNames[i], freezeSuffixes[i]);
        freezeSliderAttachments[i] = std::make_unique<SA> (
            processor.apvts, freezeParameterIds[i], freezeSliders[i]);
    }

    for (size_t i = 0; i < sidechainSliders.size(); ++i)
    {
        setupSidechainSlider (sidechainSliders[i], sidechainLabels[i],
                              sidechainNames[i], sidechainSuffixes[i]);
        sidechainSliderAttachments[i] = std::make_unique<SA> (
            processor.apvts, sidechainParameterIds[i], sidechainSliders[i]);
    }

    sidechainEnable.setClickingTogglesState (true);
    sidechainEnableAttachment = std::make_unique<BA> (
        processor.apvts, Parameters::IDs::sidechainEnable, sidechainEnable);
    addAndMakeVisible (sidechainEnable);

    sidechainStatus.setText ("SC: DISCONNECTED", juce::dontSendNotification);
    sidechainStatus.setJustificationType (juce::Justification::centred);
    sidechainStatus.setColour (juce::Label::textColourId, RomanticTheme::dim);
    sidechainStatus.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    addAndMakeVisible (sidechainStatus);

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
    addAndMakeVisible (bottomBar);

    startTimerHz (20);
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

void ReverbRomanticAudioProcessorEditor::setupSidechainSlider (
    juce::Slider& slider,
    juce::Label& label,
    const juce::String& name,
    const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
    slider.setTextValueSuffix (suffix);
    slider.setMouseDragSensitivity (220);
    slider.setScrollWheelEnabled (true);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, RomanticTheme::text);
    label.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));

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

    if (! shimmerPanelBounds.isEmpty())
        drawPanel (g, shimmerPanelBounds, "SHIMMER REVERB");

    if (! freezePanelBounds.isEmpty())
        drawPanel (g, freezePanelBounds, "FREEZE / INFINITE HOLD");

    if (! sidechainPanelBounds.isEmpty())
        drawPanel (g, sidechainPanelBounds, "SIDECHAIN DUCKING");
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

    auto tools = area.removeFromBottom (38).reduced (18, 4);
    auto abArea = tools.removeFromLeft (224);
    captureAButton.setBounds (abArea.removeFromLeft (50).reduced (1));
    snapshotAButton.setBounds (abArea.removeFromLeft (32).reduced (1));
    abArea.removeFromLeft (4);
    captureBButton.setBounds (abArea.removeFromLeft (50).reduced (1));
    snapshotBButton.setBounds (abArea.removeFromLeft (32).reduced (1));
    abArea.removeFromLeft (4);
    undoButton.setBounds (abArea.removeFromLeft (52).reduced (1));
    redoButton.setBounds (abArea.removeFromLeft (52).reduced (1));

    tools.removeFromLeft (8);
    favouriteButton.setBounds (tools.removeFromLeft (48).reduced (1));
    tools.removeFromLeft (6);
    saveUserButton.setBounds (tools.removeFromRight (50).reduced (1));
    loadUserButton.setBounds (tools.removeFromRight (50).reduced (1));
    renameUserButton.setBounds (tools.removeFromRight (44).reduced (1));
    duplicateUserButton.setBounds (tools.removeFromRight (44).reduced (1));
    deleteUserButton.setBounds (tools.removeFromRight (44).reduced (1));
    userPresetBox.setBounds (tools.reduced (2, 0));

    auto header = area.removeFromTop (88).reduced (22, 10);
    auto brand = header.removeFromLeft (juce::jmin (350, header.getWidth() / 3));
    title.setBounds (brand.removeFromTop (42));
    subtitle.setBounds (brand.removeFromTop (20));

    auto controls = header.reduced (4, 2);
    auto bypassArea = controls.removeFromRight (100);
    bypass.setBounds (bypassArea.reduced (4, 13));
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

    sidechainPanelBounds = area.removeFromBottom (122).reduced (18, 6);
    {
        auto inner = sidechainPanelBounds.reduced (10);
        inner.removeFromTop (30);

        auto statusArea = inner.removeFromLeft (132);
        sidechainEnable.setBounds (statusArea.removeFromTop (34).reduced (4, 2));
        sidechainStatus.setBounds (statusArea.removeFromTop (30).reduced (2, 1));

        inner.removeFromLeft (8);
        const int cellWidth = juce::jmax (1, inner.getWidth() / 5);

        for (size_t i = 0; i < sidechainSliders.size(); ++i)
        {
            auto cell = inner.removeFromLeft (i + 1 == sidechainSliders.size()
                                                ? inner.getWidth()
                                                : cellWidth);
            sidechainLabels[i].setBounds (cell.removeFromTop (18));
            sidechainSliders[i].setBounds (cell.reduced (3, 0));
        }
    }

    freezePanelBounds = area.removeFromBottom (122).reduced (18, 6);
    {
        auto inner = freezePanelBounds.reduced (10);
        inner.removeFromTop (30);

        auto enableArea = inner.removeFromLeft (132);
        freeze.setBounds (enableArea.removeFromTop (38).reduced (4, 2));

        inner.removeFromLeft (8);
        const int cellWidth = juce::jmax (1, inner.getWidth() / 3);

        for (size_t i = 0; i < freezeSliders.size(); ++i)
        {
            auto cell = inner.removeFromLeft (i + 1 == freezeSliders.size()
                                                ? inner.getWidth()
                                                : cellWidth);
            freezeLabels[i].setBounds (cell.removeFromTop (18));
            freezeSliders[i].setBounds (cell.reduced (3, 0));
        }
    }

    shimmerPanelBounds = area.removeFromBottom (122).reduced (18, 6);
    {
        auto inner = shimmerPanelBounds.reduced (10);
        inner.removeFromTop (30);

        auto enableArea = inner.removeFromLeft (132);
        shimmerEnable.setBounds (enableArea.removeFromTop (38).reduced (4, 2));

        inner.removeFromLeft (8);
        const int cellWidth = juce::jmax (1, inner.getWidth() / 4);

        for (size_t i = 0; i < shimmerSliders.size(); ++i)
        {
            auto cell = inner.removeFromLeft (cellWidth);
            shimmerLabels[i].setBounds (cell.removeFromTop (18));
            shimmerSliders[i].setBounds (cell.reduced (3, 0));
        }

        shimmerPitchLabel.setBounds (inner.removeFromTop (18));
        shimmerPitchBox.setBounds (inner.removeFromTop (34).reduced (6, 2));
    }

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

}

void ReverbRomanticAudioProcessorEditor::timerCallback()
{
    inputMeter.setLevels (processor.getInputPeakLeft(), processor.getInputPeakRight());
    outputMeter.setLevels (processor.getOutputPeakLeft(), processor.getOutputPeakRight());
    undoButton.setEnabled (processor.canUndoPresetChange());
    redoButton.setEnabled (processor.canRedoPresetChange());
    snapshotAButton.setToggleState (! processor.isSnapshotBActive(), juce::dontSendNotification);
    snapshotBButton.setToggleState (processor.isSnapshotBActive(), juce::dontSendNotification);

    const bool connected = processor.isSidechainConnected();
    const float peak = processor.getSidechainPeak();
    const bool detecting = connected && peak > 0.001f;

    sidechainStatus.setText (
        connected
            ? (detecting ? "SC: DETECTING" : "SC: CONNECTED")
            : "SC: DISCONNECTED",
        juce::dontSendNotification);

    sidechainStatus.setColour (
        juce::Label::textColourId,
        detecting ? RomanticTheme::accentBright
                  : (connected ? RomanticTheme::text : RomanticTheme::dim));
}

void ReverbRomanticAudioProcessorEditor::refreshUserPresetList()
{
    const auto names = processor.getUserPresetNames();
    userPresetBox.clear (juce::dontSendNotification);
    userPresetBox.addItemList (names, 1);
    if (selectedUserPreset.isNotEmpty() && names.contains (selectedUserPreset))
        userPresetBox.setText (selectedUserPreset, juce::dontSendNotification);
}

void ReverbRomanticAudioProcessorEditor::updateFavouriteButton()
{
    const int index = processor.getCurrentFactoryPreset();
    const bool favourite = index >= 0 && processor.isFactoryPresetFavourite (index);
    favouriteButton.setButtonText (favourite ? "FAV*" : "FAV");
    favouriteButton.setToggleState (favourite, juce::dontSendNotification);
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
            || (selectedCategory == "Favorites" && processor.isFactoryPresetFavourite (index))
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
    {
        processor.loadFactoryPreset (visiblePresetIndexes[static_cast<size_t> (visibleIndex)]);
        updateFavouriteButton();
    }
}

void ReverbRomanticAudioProcessorEditor::selectProcessorPreset()
{
    const int processorIndex = processor.getCurrentFactoryPreset();
    const auto found = std::find (visiblePresetIndexes.begin(), visiblePresetIndexes.end(), processorIndex);
    if (found != visiblePresetIndexes.end())
        presetBox.setSelectedItemIndex (static_cast<int> (std::distance (visiblePresetIndexes.begin(), found)),
                                        juce::dontSendNotification);
}
