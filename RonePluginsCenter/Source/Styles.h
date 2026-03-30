#pragma once
#include <JuceHeader.h>

// ============================================================================
// RONE Plugins Center — Purple/Neon Magenta palette
// Matched to rone-flanger NeonLookAndFeel + ReverseReverb CSS theme
// ============================================================================
namespace Colours_RONE
{
    // Backgrounds (deep purple family — from rone-flanger #0D0520 / #1A0A2E)
    static const juce::Colour background     { 0xff0D0520 };  // deep dark purple
    static const juce::Colour cardBackground { 0xff1A0A2E };  // dark purple panel
    static const juce::Colour cardBorder     { 0xff2A1040 };  // subtle purple edge
    static const juce::Colour headerBg       { 0xff0A0318 };  // deepest purple header

    // Accent colours (purple/magenta dominant)
    static const juce::Colour hotPurple      { 0xffb537f2 };  // primary action — ReverseReverb hot-pink
    static const juce::Colour neonPink       { 0xffE040FB };  // neon magenta — rone-flanger thumb
    static const juce::Colour neonPurple     { 0xff9d4edd };  // decorative purple
    static const juce::Colour deepPurple     { 0xff7B1FA2 };  // fill / hover
    static const juce::Colour lightPurple    { 0xffCE93D8 };  // installed state / light accent
    static const juce::Colour buttonBase     { 0xff4A148C };  // button off state
    static const juce::Colour borderPurple   { 0xff9C27B0 };  // medium purple border
    static const juce::Colour errorRed       { 0xffe94560 };  // error / cancel

    // Text
    static const juce::Colour textPrimary    { 0xffe0e0e0 };
    static const juce::Colour textSecondary  { 0xff8888aa };
    static const juce::Colour textDim        { 0xff555570 };

    // Progress bar
    static const juce::Colour progressTrack  { 0xff1a0a30 };
    static const juce::Colour progressFill   { 0xffE040FB };  // neon pink
}

// ============================================================================
// Custom LookAndFeel — matched to RONE plugin family
// ============================================================================
class RoneLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RoneLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId,  Colours_RONE::background);
        setColour (juce::TextButton::buttonColourId,           Colours_RONE::buttonBase);
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

        if (isDown)             colour = colour.darker (0.3f);
        else if (isHighlighted) colour = colour.brighter (0.15f);

        g.setColour (colour);
        g.fillRoundedRectangle (bounds, 6.0f);

        // Glow border
        g.setColour (colour.brighter (0.4f).withAlpha (0.45f));
        g.drawRoundedRectangle (bounds, 6.0f, 1.5f);
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

            // Neon glow bloom
            g.setColour (Colours_RONE::progressFill.withAlpha (0.25f));
            g.fillRoundedRectangle (bounds.withWidth (fillWidth).expanded (0, 3), 6.0f);
        }
    }
};
