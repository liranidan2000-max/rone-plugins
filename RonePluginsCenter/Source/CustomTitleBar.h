#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ============================================================================
// CustomTitleBar — the graphite window bar for the frameless window.
//
// This has to be a native JUCE component rather than part of the React UI:
// the WebView is a child Win32 window that captures the mouse, so a drag
// started from inside the page can never take over the window.
// ============================================================================
class CustomTitleBar : public juce::Component
{
public:
    static constexpr int kHeight = 36;

    CustomTitleBar()
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
        setWantsKeyboardFocus (false);
    }

    void setWindowToDrag (juce::Component* w) { windowToDrag = w; }

    // Set by MainComponent: hands the drag to the OS (Aero snap, multi-monitor).
    // Returns true if the OS took over, in which case we do nothing further.
    std::function<bool()> startNativeDrag;

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        g.setColour (juce::Colour (0xff17191E));            // panel
        g.fillRect (bounds);

        g.setColour (juce::Colour (0xff23262C));            // border
        g.fillRect (bounds.removeFromBottom (1));

        g.setColour (juce::Colour (0xff4E535B));            // faint
        g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        drawTracked (g, "RONE PLUGINS CENTER", 16.0f, 2.6f);

        drawButton (g, getMinimizeBounds(), minimizeHovered, false);
        drawButton (g, getCloseBounds(),    closeHovered,    true);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto* dw = dynamic_cast<juce::DocumentWindow*> (windowToDrag);

        if (getCloseBounds().contains (e.getPosition()))
        {
            if (dw != nullptr) dw->closeButtonPressed();
            return;
        }

        if (getMinimizeBounds().contains (e.getPosition()))
        {
            if (dw != nullptr) dw->minimiseButtonPressed();
            return;
        }

        // Prefer a real OS window drag so snapping and multi-monitor work
        draggingNatively = (startNativeDrag != nullptr && startNativeDrag());

        if (! draggingNatively && windowToDrag != nullptr)
            dragger.startDraggingComponent (windowToDrag,
                                            e.getEventRelativeTo (windowToDrag));
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (draggingNatively || windowToDrag == nullptr)
            return;

        dragger.dragComponent (windowToDrag,
                               e.getEventRelativeTo (windowToDrag), nullptr);
    }

    void mouseUp (const juce::MouseEvent&) override { draggingNatively = false; }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const bool newClose = getCloseBounds().contains (e.getPosition());
        const bool newMin   = getMinimizeBounds().contains (e.getPosition());

        if (newClose != closeHovered || newMin != minimizeHovered)
        {
            closeHovered    = newClose;
            minimizeHovered = newMin;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (closeHovered || minimizeHovered)
        {
            closeHovered = minimizeHovered = false;
            repaint();
        }
    }

private:
    static constexpr int kButtonWidth = 48;

    juce::Rectangle<int> getCloseBounds() const
    {
        return { getWidth() - kButtonWidth, 0, kButtonWidth, getHeight() - 1 };
    }

    juce::Rectangle<int> getMinimizeBounds() const
    {
        return getCloseBounds().translated (-kButtonWidth, 0);
    }

    // JUCE has no letter-spacing, so lay the label out a glyph at a time
    void drawTracked (juce::Graphics& g, const juce::String& text,
                      float x, float tracking) const
    {
        const auto font = g.getCurrentFont();

        for (auto character : text)
        {
            const auto glyph = juce::String::charToString (character);

            g.drawText (glyph, juce::Rectangle<float> (x, 0.0f, 40.0f, (float) getHeight()),
                        juce::Justification::centredLeft, false);

            x += juce::GlyphArrangement::getStringWidth (font, glyph) + tracking;
        }
    }

    void drawButton (juce::Graphics& g, juce::Rectangle<int> r,
                     bool hovered, bool isClose) const
    {
        if (hovered)
        {
            g.setColour (isClose ? juce::Colour (0xffF43F5E)
                                 : juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.06f));
            g.fillRect (r);
        }

        g.setColour (hovered ? (isClose ? juce::Colours::white : juce::Colour (0xffE8EAED))
                             : juce::Colour (0xff7A7F88));

        const auto rf = r.toFloat();
        const auto cx = rf.getCentreX();
        const auto cy = rf.getCentreY();
        const float s = 5.0f;

        if (isClose)
        {
            g.drawLine (cx - s, cy - s, cx + s, cy + s, 1.4f);
            g.drawLine (cx + s, cy - s, cx - s, cy + s, 1.4f);
        }
        else
        {
            g.drawLine (cx - s, cy, cx + s, cy, 1.4f);
        }
    }

    juce::ComponentDragger dragger;
    juce::Component* windowToDrag = nullptr;
    bool closeHovered = false;
    bool minimizeHovered = false;
    bool draggingNatively = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomTitleBar)
};
