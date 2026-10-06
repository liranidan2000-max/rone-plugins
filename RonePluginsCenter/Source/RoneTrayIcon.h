#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
#include "AutoStart.h"
#include "MainComponent.h"

class RoneTrayIcon : public juce::SystemTrayIconComponent,
                     private juce::Timer
{
public:
    explicit RoneTrayIcon (juce::DocumentWindow& window)
        : mainWindow (window)
    {
        baseImage = juce::ImageCache::getFromMemory (BinaryData::AppIcon_png, BinaryData::AppIcon_pngSize)
                        .rescaled (22, 22, juce::Graphics::highResamplingQuality);
        setUpdateCount (0);
    }

    // Center 2.0: the Center lives in the tray for days, so the tray says when
    // updates wait - an amber dot on the icon, the count in the tooltip, and
    // (Windows) one notification when the number goes up.
    void setUpdateCount (int n)
    {
        auto image = baseImage.createCopy();
        if (n > 0)
        {
            juce::Graphics g (image);
            const float d = 9.0f;
            const juce::Rectangle<float> dot ((float) image.getWidth() - d, 0.0f, d, d);
            g.setColour (juce::Colour (0xff14161A));
            g.fillEllipse (dot.expanded (1.2f));
            g.setColour (juce::Colour (0xffFFD02B));
            g.fillEllipse (dot);
        }
       #if JUCE_MAC
        setIconImage (image, menuBarTemplate (n > 0));
       #else
        setIconImage (image, image);
       #endif
        setIconTooltip (n > 0 ? "RONE Plugins Center - " + juce::String (n) + (n == 1 ? " update ready" : " updates ready")
                              : juce::String ("RONE Plugins Center"));

       #if JUCE_WINDOWS
        if (n > shownCount && ! mainWindow.isVisible())
            showInfoBubble ("RONE Plugins Center",
                            n == 1 ? juce::String ("1 plugin update is ready.")
                                   : juce::String (n) + " plugin updates are ready.");
       #endif
        shownCount = n;
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
   #if JUCE_MAC
    // The macOS menu bar uses ONLY this image, as a template: its alpha in the
    // menu bar's own colour, squeezed to 20 x 20 pt (juce_SystemTrayIcon_mac).
    // The app icon there was an opaque rounded square - a white block (Zanon,
    // 2026-10-06). This is the R mark alone, the path every plugin header draws,
    // with a heavier line so it reads at 20 pt, and a dot when updates wait.
    static juce::Image menuBarTemplate (bool dot)
    {
        constexpr int size = 64;   // shown at 20 pt: sharp on Retina
        juce::Image img (juce::Image::ARGB, size, size, true);
        juce::Graphics g (img);
        g.setColour (juce::Colours::black);

        const auto line = juce::Drawable::parseSVGPath (
            "M 0 2.1 H 46.7 A 10.5 10.5 0 0 1 46.7 23.1 H 11.6 A 6.9 6.9 0 0 0 11.6 36.9 H 34 C 46 36.9 47 48.2 59 48.2 H 100");
        juce::Path mark;
        juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt).createStrokedPath (mark, line);
        mark.addPath (juce::Drawable::parseSVGPath ("M 0 0 L 4.9 0 L 4.9 16.4 L 0 19.2 Z"));

        g.fillPath (mark, mark.getTransformToScaleToFit ({ 2.0f, 2.0f, size - 4.0f, size - 4.0f },
                                                         true, juce::Justification::centred));
        if (dot)
            g.fillEllipse (size - 17.0f, 1.0f, 16.0f, 16.0f);
        return img;
    }
   #endif

    void showWindow()
    {
        mainWindow.setVisible (true);
        mainWindow.setMinimised (false);
        mainWindow.toFront (true);
    }

    void timerCallback() override
    {
        stopTimer();

        juce::PopupMenu menu;
        menu.addItem (1, "Open RONE Center");
        menu.addSeparator();
       #if JUCE_MAC
        menu.addItem (3, "Open at login", AutoStart::isSupported(), AutoStart::isEnabled());
       #else
        menu.addItem (3, "Start with Windows", AutoStart::isSupported(), AutoStart::isEnabled());
       #endif
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
            quitAsking();
        else if (id == 3)
            AutoStart::setEnabled (! AutoStart::isEnabled());
    }

    // Quitting while something downloads or waits for a DAW drops it (an install
    // already running finishes on its own). Ask first, only then.
    void quitAsking()
    {
        auto* page = dynamic_cast<MainComponent*> (mainWindow.getContentComponent());
        if (page == nullptr || ! page->isWorking())
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            return;
        }

        juce::NativeMessageBox::showAsync (
            juce::MessageBoxOptions()
                .withIconType (juce::MessageBoxIconType::QuestionIcon)
                .withTitle ("RONE Plugins Center")
                .withMessage ("Plugins are still downloading or waiting to install. Quit anyway? "
                              "What is not installed yet will be offered again next time.")
                .withButton ("Quit")
                .withButton ("Keep running"),
            [] (int result)
            {
                if (result == 0)
                    juce::JUCEApplication::getInstance()->systemRequestedQuit();
            });
    }

    juce::DocumentWindow& mainWindow;
    juce::Image baseImage;
    int shownCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoneTrayIcon)
};
