#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Standalone-only frameless title bar (RONE house pattern, sync-verb origin)
class CustomTitleBar : public juce::Component
{
public:
    static constexpr int kHeight = 32;

    CustomTitleBar() { setMouseCursor (juce::MouseCursor::NormalCursor); }

    // AUDIO opens the standalone's audio/MIDI device settings: hiding the native title bar
    // hid JUCE's own Options button. Set by the editor.
    std::function<void()> onAudioSettings;

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
        g.drawText ("RONE AFTERSPACE", 14, 0, getAudioBounds().getX() - 22, getHeight(),
                     juce::Justification::centredLeft);

        drawAudioButton (g, getAudioBounds(), audioHovered);
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

        if (getAudioBounds().contains (e.getPosition()))
        {
            if (onAudioSettings != nullptr)
                onAudioSettings();
            return;
        }

        if (windowToDrag != nullptr)
            dragger.startDraggingComponent (windowToDrag, e.getEventRelativeTo (windowToDrag));
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (windowToDrag != nullptr
            && ! getCloseBounds().contains (e.getMouseDownPosition())
            && ! getMinimizeBounds().contains (e.getMouseDownPosition())
            && ! getAudioBounds().contains (e.getMouseDownPosition()))
            dragger.dragComponent (windowToDrag, e.getEventRelativeTo (windowToDrag), nullptr);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        bool overClose = getCloseBounds().contains (e.getPosition());
        bool overMin   = getMinimizeBounds().contains (e.getPosition());
        bool overAudio = getAudioBounds().contains (e.getPosition());
        if (overClose != closeHovered || overMin != minimizeHovered || overAudio != audioHovered)
        {
            closeHovered = overClose;
            minimizeHovered = overMin;
            audioHovered = overAudio;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        closeHovered = minimizeHovered = audioHovered = false;
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

    static constexpr int kAudioWidth = 60;

    juce::Rectangle<int> getAudioBounds() const
    {
        return { getWidth() - kHeight * 2 - kAudioWidth, 0, kAudioWidth, kHeight };
    }

    void drawAudioButton (juce::Graphics& g, juce::Rectangle<int> b, bool hovered)
    {
        if (hovered)
        {
            g.setColour (juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.08f));
            g.fillRect (b);
        }

        g.setColour (juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, hovered ? 0.95f : 0.6f));
        g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
        g.drawText ("AUDIO", b, juce::Justification::centred);
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
    bool closeHovered = false, minimizeHovered = false, audioHovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomTitleBar)
};
