#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Standalone-only frameless title bar (RONE house pattern, sync-verb origin)
class CustomTitleBar : public juce::Component
{
public:
    static constexpr int kHeight = 32;

    CustomTitleBar() { setMouseCursor (juce::MouseCursor::NormalCursor); }

    void setWindowToDrag (juce::Component* w) { windowToDrag = w; }
    juce::Component* getWindowToDrag() const  { return windowToDrag; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        g.setColour (juce::Colour (0xff101216));
        g.fillRect (bounds);

        g.setColour (juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.06f));
        g.fillRect (bounds.removeFromBottom (1));

        g.setColour (juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.85f));
        g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
        g.drawText ("RONE AFTERSPACE", 14, 0, getWidth() - 80, getHeight(),
                     juce::Justification::centredLeft);

        drawButton (g, getMinimizeBounds(), minimizeHovered, false);
        drawButton (g, getCloseBounds(),    closeHovered,    true);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (getCloseBounds().contains (e.getPosition()))
        {
            if (auto* dw = dynamic_cast<juce::DocumentWindow*> (windowToDrag))
                dw->closeButtonPressed();
            return;
        }

        if (getMinimizeBounds().contains (e.getPosition()))
        {
            if (auto* dw = dynamic_cast<juce::DocumentWindow*> (windowToDrag))
                dw->minimiseButtonPressed();
            return;
        }

        if (windowToDrag != nullptr)
            dragger.startDraggingComponent (windowToDrag, e.getEventRelativeTo (windowToDrag));
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (windowToDrag != nullptr
            && ! getCloseBounds().contains (e.getMouseDownPosition())
            && ! getMinimizeBounds().contains (e.getMouseDownPosition()))
            dragger.dragComponent (windowToDrag, e.getEventRelativeTo (windowToDrag), nullptr);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        bool overClose = getCloseBounds().contains (e.getPosition());
        bool overMin   = getMinimizeBounds().contains (e.getPosition());
        if (overClose != closeHovered || overMin != minimizeHovered)
        {
            closeHovered = overClose;
            minimizeHovered = overMin;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        closeHovered = minimizeHovered = false;
        repaint();
    }

private:
    juce::Rectangle<int> getCloseBounds() const
    {
        return { getWidth() - kHeight, 0, kHeight, kHeight };
    }

    juce::Rectangle<int> getMinimizeBounds() const
    {
        return { getWidth() - kHeight * 2, 0, kHeight, kHeight };
    }

    void drawButton (juce::Graphics& g, juce::Rectangle<int> b, bool hovered, bool isClose)
    {
        if (hovered)
        {
            g.setColour (isClose ? juce::Colour (0xffc4314b)
                                 : juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.08f));
            g.fillRect (b);
        }

        g.setColour (juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, hovered ? 0.95f : 0.6f));
        auto c = b.getCentre();

        if (isClose)
        {
            g.drawLine ((float) c.x - 5, (float) c.y - 5, (float) c.x + 5, (float) c.y + 5, 1.2f);
            g.drawLine ((float) c.x - 5, (float) c.y + 5, (float) c.x + 5, (float) c.y - 5, 1.2f);
        }
        else
        {
            g.drawLine ((float) c.x - 5, (float) c.y, (float) c.x + 5, (float) c.y, 1.2f);
        }
    }

    juce::Component* windowToDrag = nullptr;
    juce::ComponentDragger dragger;
    bool closeHovered = false, minimizeHovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomTitleBar)
};
