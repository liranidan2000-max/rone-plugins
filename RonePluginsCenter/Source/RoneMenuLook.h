#pragma once

#include <JuceHeader.h>

// ============================================================================
// RoneMenuLook - the tray menu in the house look: a dark rounded panel with a
// soft shadow, the R mark on top, an icon on every row. The same file sits in
// RonePluginsCenter/Source and RoneVstManager/Source (one is a copy of the other).
//
// A non-opaque background makes JUCE give the menu a transparent window, so the
// corners really are round. The shadow is drawn here, inside the menu's border:
// JUCE's own one is a rectangle around that window.
// ============================================================================

namespace rone
{
    /** The R mark every plugin header draws. */
    inline juce::Path markPath()
    {
        const auto line = juce::Drawable::parseSVGPath (
            "M 0 2.1 H 46.7 A 10.5 10.5 0 0 1 46.7 23.1 H 11.6 A 6.9 6.9 0 0 0 11.6 36.9 H 34 C 46 36.9 47 48.2 59 48.2 H 100");
        juce::Path mark;
        juce::PathStrokeType (4.2f, juce::PathStrokeType::curved, juce::PathStrokeType::butt).createStrokedPath (mark, line);
        mark.addPath (juce::Drawable::parseSVGPath ("M 0 0 L 4.9 0 L 4.9 16.4 L 0 19.2 Z"));
        return mark;
    }

    /** A row's icon, from a 24 x 24 SVG path drawn as a line. */
    inline std::unique_ptr<juce::Drawable> menuIcon (const char* svg, juce::Colour colour = juce::Colour (0xffC9CDD4))
    {
        juce::Path line;
        juce::PathStrokeType (1.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)
            .createStrokedPath (line, juce::Drawable::parseSVGPath (svg));
        auto d = std::make_unique<juce::DrawablePath>();
        d->setPath (line);
        d->setFill (colour);
        return d;
    }

    class MenuLook : public juce::LookAndFeel_V4
    {
    public:
        MenuLook()
        {
            setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xf714161A));
            setColour (juce::PopupMenu::textColourId, juce::Colour (0xffE9EBEF));
            setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        }

        static constexpr int shadow = 12;      // the clear margin around the panel
        static constexpr float radius = 11.0f;
        static constexpr int inset = shadow + 6;   // JUCE lays items edge to edge: they keep off the panel's sides here

        static juce::Font font (float size, bool bold)
        {
           #if JUCE_WINDOWS
            return juce::Font (juce::FontOptions ("Segoe UI", size, juce::Font::plain).withStyle (bold ? "Bold" : "Semibold"));
           #else
            return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
           #endif
        }

        int  getMenuWindowFlags() override              { return 0; }
        int  getPopupMenuBorderSize() override          { return shadow + 6; }
        juce::Font getPopupMenuFont() override          { return font (14.0f, false); }

        void drawPopupMenuBackgroundWithOptions (juce::Graphics& g, int width, int height, const juce::PopupMenu::Options&) override
        {
            g.fillAll (juce::Colours::transparentBlack);
            const auto r = juce::Rectangle<float> ((float) width, (float) height).reduced ((float) shadow);
            juce::Path panel;
            panel.addRoundedRectangle (r, radius);
            juce::DropShadow (juce::Colours::black.withAlpha (0.55f), shadow, { 0, 3 }).drawForPath (g, panel);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1C1F25), r.getTopLeft(), juce::Colour (0xff121418), r.getBottomLeft(), false));
            g.fillPath (panel);
            g.setColour (juce::Colours::white.withAlpha (0.11f));
            g.drawRoundedRectangle (r.reduced (0.5f), radius - 0.5f, 1.0f);
        }

        void getIdealPopupMenuItemSize (const juce::String& text, bool separator, int, int& w, int& h) override
        {
            if (separator)
            {
                w = 40 + 2 * inset; h = 11;
                return;
            }
            w = juce::GlyphArrangement::getStringWidthInt (getPopupMenuFont(), text) + 38 + 44 + 2 * inset;
            h = 34;
        }

        void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& full, bool separator, bool active,
                                bool highlighted, bool ticked, bool, const juce::String& text, const juce::String& shortcut,
                                const juce::Drawable* icon, const juce::Colour* colour) override
        {
            const auto area = full.reduced (inset, 0);
            if (separator)
            {
                g.setColour (juce::Colours::white.withAlpha (0.09f));
                g.fillRect (area.reduced (8, 0).withHeight (1).withCentre (area.getCentre()));
                return;
            }

            auto r = area.toFloat();
            const auto ink = colour != nullptr ? *colour : findColour (juce::PopupMenu::textColourId);
            if (highlighted && active)
            {
                g.setColour (ink.withAlpha (0.13f));
                g.fillRoundedRectangle (r, 7.0f);
            }

            auto row = area;
            auto iconBox = row.removeFromLeft (38).toFloat();
            if (icon != nullptr)
                icon->drawWithin (g, iconBox.withSizeKeepingCentre (17.0f, 17.0f), juce::RectanglePlacement::centred, active ? 1.0f : 0.4f);

            auto tail = row.removeFromRight (12);
            juce::ignoreUnused (tail);
            g.setFont (getPopupMenuFont());
            g.setColour (active ? (highlighted ? ink.brighter (0.4f) : ink) : ink.withAlpha (0.4f));

            if (ticked)
            {
                juce::Path tick;
                tick.startNewSubPath (0.0f, 5.0f); tick.lineTo (4.0f, 9.0f); tick.lineTo (12.0f, 0.5f);
                auto box = row.removeFromRight (16).toFloat().withSizeKeepingCentre (12.0f, 10.0f);
                g.saveState();
                g.setColour (juce::Colour (0xff5BD2A0));
                g.strokePath (tick, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                              juce::AffineTransform::translation (box.getX(), box.getY()));
                g.restoreState();
            }
            if (shortcut.isNotEmpty())
            {
                const auto keep = g.getCurrentFont();
                g.setFont (font (12.0f, false));
                const auto c = g.getCurrentFont();
                auto box = row.removeFromRight (juce::GlyphArrangement::getStringWidthInt (c, shortcut) + 4);
                g.setColour (juce::Colours::white.withAlpha (0.42f));
                g.drawFittedText (shortcut, box, juce::Justification::centredRight, 1);
                g.setFont (keep);
                g.setColour (active ? (highlighted ? ink.brighter (0.4f) : ink) : ink.withAlpha (0.4f));
                row.removeFromRight (14);
            }
            g.drawFittedText (text, row, juce::Justification::centredLeft, 1);
        }

        JUCE_DECLARE_NON_COPYABLE (MenuLook)
    };

    /** The menu's top row: the R mark and the app's name. Clicking it does nothing. */
    class MenuHeader : public juce::PopupMenu::CustomComponent
    {
    public:
        MenuHeader (juce::String first, juce::String rest)
            : juce::PopupMenu::CustomComponent (false), a (std::move (first)), b (std::move (rest)) {}

        void getIdealSize (int& w, int& h) override
        {
            w = 240 + 2 * MenuLook::inset; h = 44;
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().withTrimmedLeft (MenuLook::inset + 11).withTrimmedBottom (3);
            auto mark = markPath();
            auto box = r.removeFromLeft (30).toFloat().withSizeKeepingCentre (30.0f, 15.0f);
            g.setColour (juce::Colours::white);
            g.fillPath (mark, mark.getTransformToScaleToFit (box, true));

            r.removeFromLeft (11);
            auto f = MenuLook::font (12.5f, true).withExtraKerningFactor (0.14f);
            g.setFont (f);
            g.drawText (a, r, juce::Justification::centredLeft);
            r.removeFromLeft (juce::GlyphArrangement::getStringWidthInt (f, a) + 8);
            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.drawText (b, r, juce::Justification::centredLeft);
        }

    private:
        juce::String a, b;
    };
}
