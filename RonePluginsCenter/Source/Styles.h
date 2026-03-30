#pragma once
#include <JuceHeader.h>

// ============================================================================
// RONE Plugins Center — Dark Psytrance colour palette
// ============================================================================
namespace Colours_RONE
{
    // Backgrounds
    static const juce::Colour background     { 0xff0d0d1a };  // deepest dark
    static const juce::Colour cardBackground { 0xff141428 };  // card body
    static const juce::Colour cardBorder     { 0xff1e1e3a };  // subtle edge
    static const juce::Colour headerBg       { 0xff0a0a16 };  // top bar

    // Accent colours
    static const juce::Colour neonGreen      { 0xff00ff88 };  // installed / OK
    static const juce::Colour neonOrange     { 0xffff9f1c };  // update available
    static const juce::Colour neonBlue       { 0xff2ec4b6 };  // install / action
    static const juce::Colour neonPink       { 0xffe94560 };  // error / cancel
    static const juce::Colour neonPurple     { 0xff7b2dff };  // decorative glow

    // Text
    static const juce::Colour textPrimary    { 0xffe0e0e8 };
    static const juce::Colour textSecondary  { 0xff8888a0 };
    static const juce::Colour textDim        { 0xff555570 };

    // Progress bar
    static const juce::Colour progressTrack  { 0xff1a1a30 };
    static const juce::Colour progressFill   { 0xff00ff88 };
}

// ============================================================================
// Custom LookAndFeel
// ============================================================================
class RoneLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RoneLookAndFeel()
    {
        // Global colour overrides
        setColour (juce::ResizableWindow::backgroundColourId,  Colours_RONE::background);
        setColour (juce::TextButton::buttonColourId,           Colours_RONE::neonBlue);
        setColour (juce::TextButton::textColourOffId,          juce::Colours::white);
        setColour (juce::Label::textColourId,                  Colours_RONE::textPrimary);
        setColour (juce::ProgressBar::foregroundColourId,      Colours_RONE::progressFill);
        setColour (juce::ProgressBar::backgroundColourId,      Colours_RONE::progressTrack);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& bgColour,
                               bool isHighlighted, bool isDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        auto colour = bgColour;

        if (isDown)        colour = colour.darker (0.3f);
        else if (isHighlighted) colour = colour.brighter (0.15f);

        g.setColour (colour);
        g.fillRoundedRectangle (bounds, 6.0f);

        // Subtle glow border
        g.setColour (colour.brighter (0.4f).withAlpha (0.3f));
        g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
    }

    void drawProgressBar (juce::Graphics& g, juce::ProgressBar& bar,
                          int width, int height, double progress,
                          const juce::String& /*text*/) override
    {
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

        // Track
        g.setColour (Colours_RONE::progressTrack);
        g.fillRoundedRectangle (bounds, 4.0f);

        // Fill
        if (progress > 0.0)
        {
            auto fillWidth = bounds.getWidth() * (float) progress;
            g.setColour (Colours_RONE::progressFill);
            g.fillRoundedRectangle (bounds.withWidth (fillWidth), 4.0f);

            // Glow
            g.setColour (Colours_RONE::progressFill.withAlpha (0.15f));
            g.fillRoundedRectangle (bounds.withWidth (fillWidth).expanded (0, 2), 6.0f);
        }
    }
};
