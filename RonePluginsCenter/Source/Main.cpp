#include <JuceHeader.h>
#include "MainComponent.h"
#include "RoneTrayIcon.h"
#include "AutoStart.h"

// ============================================================================
// RONE Plugins Center — Application entry point
// ============================================================================
class RonePluginsCenterApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName()    override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed()          override { return false; }

    void initialise (const juce::String& commandLine) override
    {
        // Launched by the OS at login (AutoStart): live in the tray, validate
        // the licence in the background, and only show a window when asked.
        const bool startInTray = commandLine.contains (AutoStart::kTrayFlag);

        mainWindow = std::make_unique<MainWindow> (getApplicationName(), startInTray);
        trayIcon = std::make_unique<RoneTrayIcon> (*mainWindow);

        AutoStart::applyDefaultOnce();   // on by default, once; the Settings toggle owns it afterwards
        AutoStart::refreshIfEnabled();   // an update may have moved the executable
    }

    void shutdown() override
    {
        trayIcon.reset();
        mainWindow.reset();
    }

    void anotherInstanceStarted (const juce::String&) override
    {
        if (mainWindow != nullptr)
        {
            mainWindow->setVisible (true);
            mainWindow->toFront (true);
        }
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    // ========================================================================
    // Main application window
    // ========================================================================
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name, bool startHidden = false)
            : DocumentWindow (name,
                              juce::Colour (0xff14161A),
                              DocumentWindow::allButtons)
        {
           #if JUCE_WINDOWS
            // Frameless: the React UI renders its own graphite title bar and
            // hands dragging/resizing back to the OS via native bridge calls.
            setUsingNativeTitleBar (false);
            setTitleBarHeight (0);
            setContentOwned (new MainComponent(), true);
            setResizable (true, false);
           #else
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);
            setResizable (true, true);
           #endif
            setResizeLimits (700, 500, 1400, 1000);
            centreWithSize (getWidth(), getHeight());
            setVisible (! startHidden);
        }

        void closeButtonPressed() override
        {
            setVisible (false);
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<RoneTrayIcon> trayIcon;
};

// Launch the app
START_JUCE_APPLICATION (RonePluginsCenterApp)
