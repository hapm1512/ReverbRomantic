#pragma once
#include <JuceHeader.h>
#include "Theme.h"

class FFTGraph final : public juce::Component
{
public:
    void setActivity (float newActivity)
    {
        activity = juce::jlimit (0.0f, 1.0f, newActivity);
        phase += 0.045f;
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (RomanticTheme::panelAlt.withAlpha (0.38f));
        g.fillRoundedRectangle (b, 12.0f);
        g.setColour (RomanticTheme::border.withAlpha (0.22f));
        for (int i = 1; i < 8; ++i)
        {
            const auto x = b.getX() + b.getWidth() * (float) i / 8.0f;
            g.drawVerticalLine ((int) x, b.getY(), b.getBottom());
        }
        for (int i = 1; i < 4; ++i)
        {
            const auto y = b.getY() + b.getHeight() * (float) i / 4.0f;
            g.drawHorizontalLine ((int) y, b.getX(), b.getRight());
        }
        juce::Path p;
        constexpr int points = 96;
        for (int i = 0; i < points; ++i)
        {
            const float t = (float) i / (float) (points - 1);
            const float envelope = std::exp (-2.45f * t);
            const float wave = 0.50f + 0.24f * std::sin (phase + t * 22.0f)
                                     + 0.10f * std::sin (phase * 0.63f + t * 51.0f);
            const float yNorm = juce::jlimit (0.04f, 0.94f, wave * envelope + 0.34f);
            const auto x = b.getX() + t * b.getWidth();
            const auto y = b.getBottom() - yNorm * b.getHeight() * (0.68f + activity * 0.30f);
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (RomanticTheme::accent.withAlpha (0.15f));
        auto fill = p;
        fill.lineTo (b.getRight(), b.getBottom());
        fill.lineTo (b.getX(), b.getBottom());
        fill.closeSubPath();
        g.fillPath (fill);
        g.setColour (RomanticTheme::accentBright.withAlpha (0.42f));
        g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved));
    }
private:
    float activity = 0.0f;
    float phase = 0.0f;
};
