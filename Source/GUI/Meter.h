#pragma once
#include <JuceHeader.h>
#include "Theme.h"

class Meter final : public juce::Component
{
public:
    explicit Meter (juce::String meterName) : name (std::move (meterName)) {}
    void setLevels (float left, float right)
    {
        targetL = juce::jlimit (0.0f, 1.0f, left);
        targetR = juce::jlimit (0.0f, 1.0f, right);
        displayL = juce::jmax (targetL, displayL * 0.90f);
        displayR = juce::jmax (targetR, displayR * 0.90f);
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (RomanticTheme::dim);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawFittedText (name, getLocalBounds().removeFromTop (18), juce::Justification::centred, 1);
        b.removeFromTop (20.0f);
        auto left = b.removeFromLeft (b.getWidth() * 0.46f).reduced (2.0f);
        b.removeFromLeft (2.0f);
        auto right = b.reduced (2.0f);
        drawChannel (g, left, displayL);
        drawChannel (g, right, displayR);
    }
private:
    static void drawChannel (juce::Graphics& g, juce::Rectangle<float> b, float value)
    {
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (b, 3.0f);
        const auto filled = b.withTop (b.getBottom() - b.getHeight() * value);
        juce::ColourGradient gradient (RomanticTheme::meterRed, filled.getCentreX(), b.getY(),
                                       RomanticTheme::meterGreen, filled.getCentreX(), b.getBottom(), false);
        gradient.addColour (0.32, RomanticTheme::meterYellow);
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (filled, 3.0f);
        g.setColour (RomanticTheme::border.withAlpha (0.8f));
        g.drawRoundedRectangle (b, 3.0f, 1.0f);
    }
    juce::String name;
    float targetL = 0.0f, targetR = 0.0f;
    float displayL = 0.0f, displayR = 0.0f;
};
