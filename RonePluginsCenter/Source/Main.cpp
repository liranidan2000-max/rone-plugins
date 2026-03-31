#include <JuceHeader.h>
#include "MainComponent.h"
#include "RoneTrayIcon.h"

// ============================================================================
// RONE Plugins Center — Application entry point
// ============================================================================
class RonePluginsCenterApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName()    override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed()          override { return false; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
        trayIcon = std::make_unique<RoneTrayIcon> (*mainWindow);
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
        explicit MainWindow (const juce::String& name)
            : DocumentWindow (name,
                              juce::Colour (0xff0B0416),
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);
            setResizable (true, true);
            setResizeLimits (700, 500, 1400, 1000);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);
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
