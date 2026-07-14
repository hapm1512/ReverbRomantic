#pragma once
#include <JuceHeader.h>
#include "Theme.h"

class RomanticLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    RomanticLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, RomanticTheme::text);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::backgroundColourId, RomanticTheme::panelAlt);
        setColour (juce::ComboBox::outlineColourId, RomanticTheme::border);
        setColour (juce::ComboBox::textColourId, RomanticTheme::text);
        setColour (juce::PopupMenu::backgroundColourId, RomanticTheme::panelAlt);
        setColour (juce::PopupMenu::textColourId, RomanticTheme::text);
        setColour (juce::TextButton::buttonColourId, RomanticTheme::panelAlt);
        setColour (juce::TextButton::buttonOnColourId, RomanticTheme::accent);
        setColour (juce::TextButton::textColourOffId, RomanticTheme::text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y,
                                               (float) width, (float) height).reduced (9.0f);
        const auto diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
        bounds = bounds.withSizeKeepingCentre (diameter, diameter);
        const auto centre = bounds.getCentre();
        const auto radius = diameter * 0.5f;
        const auto angle = startAngle + sliderPos * (endAngle - startAngle);

        g.setColour (juce::Colours::black.withAlpha (0.58f));
        g.fillEllipse (bounds.translated (0.0f, 4.0f));

        juce::ColourGradient body (juce::Colour::fromRGB (105, 84, 83), centre.x, bounds.getY(),
                                   juce::Colour::fromRGB (25, 21, 25), centre.x, bounds.getBottom(), false);
        g.setGradientFill (body);
        g.fillEllipse (bounds);
        g.setColour (RomanticTheme::border.withAlpha (0.9f));
        g.drawEllipse (bounds, 1.3f);

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius - 3.0f, radius - 3.0f,
                             0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved));

        juce::Path value;
        value.addCentredArc (centre.x, centre.y, radius - 3.0f, radius - 3.0f,
                             0.0f, startAngle, angle, true);
        g.setColour (RomanticTheme::accentBright);
        g.strokePath (value, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved));

        juce::Path pointer;
        pointer.addRoundedRectangle (-1.4f, -radius + 10.0f, 2.8f, radius * 0.42f, 1.3f);
        g.setColour (RomanticTheme::text);
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (RomanticTheme::accent.withAlpha (0.65f));
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (centre));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour&, bool highlighted, bool down) override
    {
        auto b = button.getLocalBounds().toFloat().reduced (0.5f);
        auto fill = button.getToggleState() ? RomanticTheme::accent : RomanticTheme::panelAlt;
        if (highlighted) fill = fill.brighter (0.10f);
        if (down) fill = fill.darker (0.12f);
        g.setColour (fill);
        g.fillRoundedRectangle (b, 7.0f);
        g.setColour (button.getToggleState() ? RomanticTheme::accentBright : RomanticTheme::border);
        g.drawRoundedRectangle (b, 7.0f, 1.0f);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox&) override
    {
        auto b = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
        g.setColour (RomanticTheme::panelAlt);
        g.fillRoundedRectangle (b, 7.0f);
        g.setColour (RomanticTheme::border);
        g.drawRoundedRectangle (b, 7.0f, 1.0f);
        juce::Path arrow;
        arrow.addTriangle ((float) width - 19.0f, (float) height * 0.42f,
                           (float) width - 11.0f, (float) height * 0.42f,
                           (float) width - 15.0f, (float) height * 0.62f);
        g.setColour (RomanticTheme::accentBright);
        g.fillPath (arrow);
    }
};
