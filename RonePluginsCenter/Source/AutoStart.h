#pragma once

#include <JuceHeader.h>

// ============================================================================
// AutoStart — "Start with Windows" / "Open at login" for the Plugins Center.
//
// Why this exists: the licence is validated by the Center (every 24 h while it
// runs, and on every launch). A Center that starts with the machine, minimised
// to the tray, keeps every plugin's BundleLicense.xml fresh without the user
// ever opening it - and makes a remote revoke bite within a day instead of
// "whenever they next open the Center".
//
// Windows: HKCU\Software\Microsoft\Windows\CurrentVersion\Run\RONE Plugins Center
//          = "<exe>" --tray
// macOS:   ~/Library/LaunchAgents/com.roneplugins.center.plist (RunAtLoad)
//
// The OS entry is the single source of truth (no separate setting file), so
// a user who removes it with Task Manager / System Settings sees the toggle
// off in the Center as well. It is switched ON once, on the first launch of a
// build that has it (marker file); after that the toggle is the user's.
// ============================================================================
namespace AutoStart
{
static constexpr const char* kTrayFlag = "--tray";

inline juce::File executable()
{
    return juce::File::getSpecialLocation (juce::File::currentExecutableFile);
}

inline juce::String command()
{
    return "\"" + executable().getFullPathName() + "\" " + kTrayFlag;
}

inline bool isSupported()
{
   #if JUCE_WINDOWS || JUCE_MAC
    return true;
   #else
    return false;
   #endif
}

#if JUCE_WINDOWS
inline juce::String registryPath()
{
    return "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\RONE Plugins Center";
}
#elif JUCE_MAC
inline juce::File launchAgent()
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
               .getChildFile ("Library/LaunchAgents/com.roneplugins.center.plist");
}
#endif

inline bool isEnabled()
{
   #if JUCE_WINDOWS
    return juce::WindowsRegistry::valueExists (registryPath());
   #elif JUCE_MAC
    return launchAgent().existsAsFile();
   #else
    return false;
   #endif
}

inline bool setEnabled (bool on)
{
   #if JUCE_WINDOWS
    if (on)
        return juce::WindowsRegistry::setValue (registryPath(), command());
    juce::WindowsRegistry::deleteValue (registryPath());
    return true;
   #elif JUCE_MAC
    auto plist = launchAgent();
    if (! on)
        return plist.deleteFile() || ! plist.exists();

    plist.getParentDirectory().createDirectory();
    const juce::String xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
        "<plist version=\"1.0\"><dict>\n"
        "  <key>Label</key><string>com.roneplugins.center</string>\n"
        "  <key>ProgramArguments</key><array>\n"
        "    <string>" + executable().getFullPathName() + "</string>\n"
        "    <string>" + juce::String (kTrayFlag) + "</string>\n"
        "  </array>\n"
        "  <key>RunAtLoad</key><true/>\n"
        "</dict></plist>\n";
    return plist.replaceWithText (xml);
   #else
    juce::ignoreUnused (on);
    return false;
   #endif
}

/** Switch it on the first time this build runs; never override a later choice. */
inline void applyDefaultOnce()
{
    if (! isSupported())
        return;

    auto marker = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                      .getChildFile ("RonePluginsCenter")
                      .getChildFile ("autostart.defaulted");
    if (marker.existsAsFile())
        return;

    marker.getParentDirectory().createDirectory();
    setEnabled (true);
    marker.replaceWithText ("1");
}

/** The exe moved (update, reinstall) - keep the entry pointing at the new path. */
inline void refreshIfEnabled()
{
    if (isEnabled())
        setEnabled (true);
}
} // namespace AutoStart
