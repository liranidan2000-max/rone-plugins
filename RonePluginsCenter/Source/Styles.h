#pragma once
#include <JuceHeader.h>

// ============================================================================
// RONE Plugins Center — Refined Purple palette
// Apple-inspired clean aesthetic with RONE signature purple accents
// ============================================================================
namespace Colours_RONE
{
    // Backgrounds — deep, layered
    static const juce::Colour background     { 0xff0B0416 };  // deepest base
    static const juce::Colour cardBackground { 0xff150A26 };  // elevated card surface
    static const juce::Colour cardBorder     { 0xff251540 };  // subtle card edge
    static const juce::Colour headerBg       { 0xff0E0620 };  // header surface
    static const juce::Colour licenseBg      { 0xff110720 };  // license bar

    // Primary accents
    static const juce::Colour hotPurple      { 0xffb537f2 };  // primary CTA
    static const juce::Colour neonPink       { 0xffE040FB };  // secondary accent
    static const juce::Colour neonPurple     { 0xff9d4edd };  // decorative
    static const juce::Colour deepPurple     { 0xff7B1FA2 };  // hover/fill
    static const juce::Colour lightPurple    { 0xffCE93D8 };  // subtle accent
    static const juce::Colour buttonBase     { 0xff3A1070 };  // button idle
    static const juce::Colour borderPurple   { 0xff9C27B0 };  // medium border
    static const juce::Colour errorRed       { 0xffe94560 };  // error state

    // Badge colours
    static const juce::Colour badgeVST3      { 0xff6A1B9A };
    static const juce::Colour badgeAU        { 0xff8E24AA };
    static const juce::Colour badgeStandalone { 0xff4A148C };
    static const juce::Colour installedGreen { 0xff00C853 };

    // Text — high contrast hierarchy
    static const juce::Colour textPrimary    { 0xfff0f0f0 };  // bright white
    static const juce::Colour textSecondary  { 0xff9999bb };  // muted
    static const juce::Colour textDim        { 0xff606080 };  // very subtle

    // Progress / track
    static const juce::Colour progressTrack  { 0xff1a0a30 };
    static const juce::Colour progressFill   { 0xffE040FB };
}

// ============================================================================
// Custom LookAndFeel — refined, Apple-inspired
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

        // Tooltip styling
        setColour (juce::TooltipWindow::backgroundColourId,    Colours_RONE::cardBackground);
        setColour (juce::TooltipWindow::textColourId,          Colours_RONE::textPrimary);
        setColour (juce::TooltipWindow::outlineColourId,       Colours_RONE::cardBorder);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& bgColour,
                               bool isHighlighted, bool isDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        auto colour = bgColour;

        if (isDown)             colour = colour.darker (0.25f);
        else if (isHighlighted) colour = colour.brighter (0.12f);

        // Soft shadow (Apple-style depth)
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRoundedRectangle (bounds.translated (0, 1), 8.0f);

        // Fill
        g.setColour (colour);
        g.fillRoundedRectangle (bounds, 8.0f);

        // Subtle top highlight
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (bounds.removeFromTop (bounds.getHeight() * 0.5f), 8.0f);
    }

    void drawProgressBar (juce::Graphics& g, juce::ProgressBar& /*bar*/,
                          int width, int height, double progress,
                          const juce::String& /*text*/) override
    {
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

        g.setColour (Colours_RONE::progressTrack);
        g.fillRoundedRectangle (bounds, 4.0f);

        if (progress > 0.0)
        {
            auto fillWidth = bounds.getWidth() * (float) progress;

            // Gradient fill
            juce::ColourGradient grad (Colours_RONE::hotPurple, 0, 0,
                                        Colours_RONE::neonPink, fillWidth, 0, false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (bounds.withWidth (fillWidth), 4.0f);
        }
    }
};
