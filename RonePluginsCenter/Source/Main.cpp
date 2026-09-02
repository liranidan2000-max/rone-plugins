#include <JuceHeader.h>
#include "MainComponent.h"
#include "RoneTrayIcon.h"
#include "AutoStart.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

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

    // A second launch (the OPEN RONE PLUGINS CENTER button on a plugin's lock
    // screen, a Start-menu click) hands its command line to us and quits.
    void anotherInstanceStarted (const juce::String&) override
    {
        if (mainWindow != nullptr)
            mainWindow->showAndRaise();
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
            setResizeLimits (980, 600, 1700, 1100);
            centreWithSize (getWidth(), getHeight());
            setVisible (! startHidden);
        }

        void closeButtonPressed() override
        {
            setVisible (false);
        }

        // Show the window and put it in front of whatever the user is in -
        // typically the DAW whose plugin just asked for us. A background
        // process may not take the foreground on Windows (the request only
        // flashes the taskbar button), so borrow the foreground thread's input
        // state for the call, the standard way round that rule.
        void showAndRaise()
        {
            setVisible (true);
            setMinimised (false);

           #if JUCE_WINDOWS
            if (auto* hwnd = (HWND) getWindowHandle())
            {
                const DWORD fgThread = GetWindowThreadProcessId (GetForegroundWindow(), nullptr);
                const DWORD myThread = GetCurrentThreadId();
                const bool attached = fgThread != 0 && fgThread != myThread
                                      && AttachThreadInput (fgThread, myThread, TRUE);
                ShowWindow (hwnd, SW_SHOW);
                SetForegroundWindow (hwnd);
                BringWindowToTop (hwnd);
                if (attached)
                    AttachThreadInput (fgThread, myThread, FALSE);
            }
           #endif

            toFront (true);
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<RoneTrayIcon> trayIcon;
};

// Launch the app
START_JUCE_APPLICATION (RonePluginsCenterApp)
