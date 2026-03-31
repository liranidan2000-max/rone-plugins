#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>

class RoneTrayIcon : public juce::SystemTrayIconComponent,
                     private juce::Timer
{
public:
    explicit RoneTrayIcon (juce::DocumentWindow& window)
        : mainWindow (window)
    {
        auto fullImage = juce::ImageCache::getFromMemory (
            BinaryData::AppIcon_png,
            BinaryData::AppIcon_pngSize);

        auto scaled = fullImage.rescaled (22, 22, juce::Graphics::highResamplingQuality);

        setIconImage (scaled, scaled);
        setIconTooltip ("RONE Plugins Center");
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        juce::Process::makeForegroundProcess();

        if (e.mods.isLeftButtonDown())
        {
            showWindow();
        }
        else if (e.mods.isRightButtonDown() || e.mods.isPopupMenu())
        {
            startTimer (50);
        }
    }

private:
    void showWindow()
    {
        mainWindow.setVisible (true);
        mainWindow.toFront (true);
    }

    void timerCallback() override
    {
        stopTimer();

        juce::PopupMenu menu;
        menu.addItem (1, "Open RONE Center");
        menu.addSeparator();
        menu.addItem (2, "Quit");

        menu.showMenuAsync (juce::PopupMenu::Options(),
            [this] (int result)
            {
                handleMenuResult (result);
            });
    }

    void handleMenuResult (int id)
    {
        if (id == 1)
            showWindow();
        else if (id == 2)
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

    juce::DocumentWindow& mainWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoneTrayIcon)
};
