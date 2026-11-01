#include <JuceHeader.h>
#include "MainComponent.h"
#include "RoneTrayIcon.h"
#include "AutoStart.h"
#include "OldVersionCleaner.h"
#include "UserFolderMove.h"
#include "PluginUninstaller.h"
#include "ControlPort.h"
#include "InstallBatcher.h"
#include "CenterLinks.h"
#include "CenterIntro.h"

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
    // The elevated DELETE OLD VERSIONS run (and its dry run), UNINSTALL and the
    // install batch must start next to the open Center.
    bool moreThanOneInstanceAllowed()          override { return OldVersionCleaner::isCommandLineMode (getCommandLineParameters())
                                                                  || UserFolderMove::isCommandLineMode (getCommandLineParameters())
                                                                  || PluginUninstaller::isCommandLineMode (getCommandLineParameters())
                                                                  || InstallBatcher::isCommandLineMode (getCommandLineParameters()); }

    void initialise (const juce::String& commandLine) override
    {
        // --delete-old-versions / --list-old-versions: no window, do it, quit.
        // --uninstall-plugin <key> <files...>: the elevated half of a card's UNINSTALL.
        // --install-batch <job>: the elevated half of every install (InstallBatcher.h).
        if (OldVersionCleaner::runCommandLine (commandLine) || PluginUninstaller::runCommandLine (commandLine)
            || InstallBatcher::runCommandLine (commandLine) || UserFolderMove::runCommandLine (commandLine))
        {
            quit();
            return;
        }

       #if JUCE_WINDOWS
        // The Center's own installer waits for this to go before it replaces the
        // exe (installer/RONE_Plugins.iss, self-update). Held until the process exits.
        runningMutex = CreateMutexW (nullptr, FALSE, L"RonePluginsCenterRunning");
       #endif

        // ronecenter:// links from the website open (or wake) this Center.
        CenterLinks::registerScheme();

        // Launched by the OS at login (AutoStart): live in the tray, validate
        // the licence in the background, and only show a window when asked.
        const auto link = CenterLinks::parse (commandLine);
        const bool startInTray = commandLine.contains (AutoStart::kTrayFlag) && ! link.isValid();

        // A plain launch: the icon intro first (CenterIntro.h). The window loads
        // its page beyond the screen's edge and opens when the intro ends with
        // the library drawn. A link or an UPDATE button goes straight in.
       #if JUCE_WINDOWS
        const bool playIntro = ! startInTray && ! link.isValid() && ! commandLine.contains ("--update");
       #else
        const bool playIntro = false;   // Windows first; the Mac keeps its plain open
       #endif

        mainWindow = std::make_unique<MainWindow> (getApplicationName(), startInTray, playIntro);
        trayIcon = std::make_unique<RoneTrayIcon> (*mainWindow);

        if (playIntro)
        {
            intro = std::make_unique<CenterIntro> (mainWindow->getOpenBounds().getCentre());
            intro->onReveal = [this] { if (mainWindow != nullptr) mainWindow->showAndRaise(); };
            intro->onDone = [this] { juce::MessageManager::callAsync ([this] { intro.reset(); }); };
            if (auto* page = dynamic_cast<MainComponent*> (mainWindow->getContentComponent()))
                page->onPageReady = [this] { if (intro != nullptr) intro->pageReady(); };
        }

        if (auto* page = dynamic_cast<MainComponent*> (mainWindow->getContentComponent()))
        {
            page->onUpdateCountChanged = [this] (int n)
            {
                if (trayIcon != nullptr)
                    trayIcon->setUpdateCount (n);
            };
            page->onTrayNotice = [this] (const juce::String& text)
            {
                if (trayIcon != nullptr)
                    trayIcon->notify (text);
            };
        }
        mainWindow->onClosedToTray = [this]
        {
            if (trayIcon != nullptr)
                trayIcon->notifyHiddenOnce();
        };

        AutoStart::applyDefaultOnce();   // on by default, once; the Settings toggle owns it afterwards
        AutoStart::refreshIfEnabled();   // an update may have moved the executable

        // RONE Control's MIDI port, open from login so FL meets it as it starts (ControlPort.h)
        controlPort = std::make_unique<ControlPort>();

        handleUpdateRequest (commandLine);
        handleLink (link);
    }

    void shutdown() override
    {
        intro.reset();
        controlPort.reset();
        trayIcon.reset();
        mainWindow.reset();

       #if JUCE_WINDOWS
        if (runningMutex != nullptr)
            CloseHandle (runningMutex);
        runningMutex = nullptr;
       #endif
    }

    // A second launch (the OPEN RONE PLUGINS CENTER button on a plugin's lock
    // screen, a Start-menu click, a ronecenter:// link) hands its command line
    // to us and quits. macOS delivers a ronecenter:// URL here too.
    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        if (intro != nullptr)
            intro->skip();   // opened again during the intro: the window now

        if (mainWindow != nullptr)
            mainWindow->showAndRaise();

        handleUpdateRequest (commandLine);
        handleLink (CenterLinks::parse (commandLine));
    }

    void handleLink (const CenterLinks::Link& link)
    {
        if (! link.isValid() || mainWindow == nullptr)
            return;

        mainWindow->showAndRaise();
        if (auto* page = dynamic_cast<MainComponent*> (mainWindow->getContentComponent()))
            page->handleLink (link);
    }

    // "--update <product id>": a plugin's UPDATE button (Shared/RoneUpdatePrompt.h).
    void handleUpdateRequest (const juce::String& commandLine)
    {
        auto tokens = juce::StringArray::fromTokens (commandLine, true);
        const int at = tokens.indexOf ("--update");
        if (at < 0 || at + 1 >= tokens.size() || mainWindow == nullptr)
            return;

        const auto productId = tokens[at + 1].unquoted().trim();
        if (productId.isEmpty() || ! productId.containsOnly ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"))
            return;

        mainWindow->showAndRaise();
        if (auto* page = dynamic_cast<MainComponent*> (mainWindow->getContentComponent()))
            page->requestUpdate (productId);
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    // ========================================================================
    // Main application window
    // ========================================================================
    class MainWindow : public juce::DocumentWindow,
                       private juce::Timer
    {
    public:
        /** `parked`: a launch with the intro - the window is shown (a hidden
            WebView2 may not load) but beyond the desktop's right edge, and
            comes to its place on the first showAndRaise(). */
        MainWindow (const juce::String& name, bool startHidden, bool startParked)
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
            // The grid flows from three to five cards a row, so a big screen is welcome.
            setResizeLimits (980, 620, 3840, 2400);
            centreWithSize (getWidth(), getHeight());
            openBounds = getBounds();
            if (startParked && ! startHidden)
            {
                parked = true;
                const auto all = juce::Desktop::getInstance().getDisplays().getTotalBounds (true);
                setTopLeftPosition (all.getRight() + 200, all.getY());
            }
            setVisible (! startHidden);
        }

        /** Where the window opens (centred), also while it is parked. */
        juce::Rectangle<int> getOpenBounds() const { return parked ? openBounds : getBounds(); }

        void closeButtonPressed() override
        {
            setVisible (false);
            if (onClosedToTray)
                onClosedToTray();
        }

        std::function<void()> onClosedToTray;   // the tray says it once (RoneTrayIcon::notifyHiddenOnce)

        // The page (WebView2) exists only while the window can be seen: hidden
        // in the tray or minimised, the Center keeps its backend and lets the
        // six msedgewebview2 processes go. Started with --tray it never makes
        // one until the window is first opened.
        void visibilityChanged() override
        {
            DocumentWindow::visibilityChanged();
            syncPage();
        }

        void minimisationStateChanged (bool isNowMinimised) override
        {
            DocumentWindow::minimisationStateChanged (isNowMinimised);
            syncPage();
        }

        // Show the window and put it in front of whatever the user is in -
        // typically the DAW whose plugin just asked for us. A background
        // process may not take the foreground on Windows (the request only
        // flashes the taskbar button), so borrow the foreground thread's input
        // state for the call, the standard way round that rule.
        void showAndRaise()
        {
            if (parked)
            {
                parked = false;
                setBounds (openBounds);
            }
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
        void syncPage()
        {
            auto* page = dynamic_cast<MainComponent*> (getContentComponent());
            if (page == nullptr)
                return;

            if (isVisible() && ! isMinimised())
            {
                releasePending = false;
                page->setUiVisible (true);
                foregroundKnown = false;
                startTimer (1000);
                return;
            }

            stopTimer();

            // Released a moment later: never from inside a WebView2 callback that
            // hid the window (the page's own minimise or close button), and not at
            // all if the window came straight back.
            if (releasePending)
                return;

            releasePending = true;
            juce::Component::SafePointer<MainWindow> safe (this);
            juce::Timer::callAfterDelay (1500, [safe]
            {
                if (safe == nullptr || ! safe->releasePending)
                    return;

                safe->releasePending = false;

                if (! safe->isVisible() || safe->isMinimised())
                    if (auto* p = dynamic_cast<MainComponent*> (safe->getContentComponent()))
                        p->setUiVisible (false);
            });
        }

        // Once a second while the page lives: is this window the one in front?
        // Asked of Windows itself, because keyboard focus inside the WebView2
        // child makes JUCE's own isActiveWindow() report false while the user
        // is clicking in the page.
        void timerCallback() override
        {
           #if JUCE_WINDOWS
            auto* hwnd = (HWND) getWindowHandle();
            const bool inFront = hwnd != nullptr && GetAncestor (GetForegroundWindow(), GA_ROOT) == hwnd;
           #else
            const bool inFront = isActiveWindow();
           #endif
            if (foregroundKnown && inFront == wasInFront)
                return;

            foregroundKnown = true;
            wasInFront = inFront;
            if (auto* page = dynamic_cast<MainComponent*> (getContentComponent()))
                page->setWindowActive (inFront);
        }

        juce::Rectangle<int> openBounds;
        bool parked = false;
        bool releasePending = false;
        bool foregroundKnown = false;
        bool wasInFront = true;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<RoneTrayIcon> trayIcon;
    std::unique_ptr<ControlPort> controlPort;
    std::unique_ptr<CenterIntro> intro;
   #if JUCE_WINDOWS
    HANDLE runningMutex = nullptr;
   #endif
};

// Launch the app
START_JUCE_APPLICATION (RonePluginsCenterApp)
