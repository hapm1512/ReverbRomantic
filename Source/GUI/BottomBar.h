#pragma once
#include <JuceHeader.h>
#include "Theme.h"
class BottomBar final : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        g.setColour (RomanticTheme::panelAlt.withAlpha (0.88f));
        g.fillRect (getLocalBounds());
        g.setColour (RomanticTheme::border.withAlpha (0.55f));
        g.drawHorizontalLine (0, 0.0f, (float) getWidth());
        g.setFont (juce::FontOptions (11.5f));
        g.setColour (RomanticTheme::dim);
        g.drawText ("HYBRID FDN 16x16", 18, 0, 220, getHeight(), juce::Justification::centredLeft);
        g.drawText ("Copyright Hai Pham", getWidth() - 220, 0, 202, getHeight(), juce::Justification::centredRight);
    }
};
