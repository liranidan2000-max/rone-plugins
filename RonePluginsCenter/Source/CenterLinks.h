#pragma once

#include <JuceHeader.h>
#include "AutoStart.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

// ============================================================================
// CenterLinks - ronecenter:// links (Center 2.0).
//
// After a purchase the website shows "Open in RONE Plugins Center": the link
// ronecenter://install/RoneIron starts (or wakes) the Center, which re-checks
// the account - the purchase is seconds old - and installs the plugin. The
// gap between paying and having the plugin in the DAW used to be "find the
// Center, sign in, find the card, press Install".
//
//   ronecenter://install/<productId>   check the account, then install it
//   ronecenter://plugin/<productId>    open its page
//   ronecenter://updates               the Updates page
//   ronecenter://open                  just show the Center
//
// Windows: the scheme is registered per user (HKCU\Software\Classes) by the
// Center itself on every start, so it always points at the exe that runs and
// needs no admin rights. macOS: CFBundleURLTypes in the app's Info.plist
// (CMakeLists.txt), and JUCE hands the URL to anotherInstanceStarted().
// ============================================================================
namespace CenterLinks
{
    static constexpr const char* kScheme = "ronecenter";
    static constexpr const char* kFlag   = "--link";

    struct Link
    {
        juce::String action;     // install | plugin | updates | open
        juce::String productId;
        bool isValid() const { return action.isNotEmpty(); }
    };

    // Finds a ronecenter:// URL anywhere in a command line (Windows passes
    // --link "ronecenter://...", macOS the bare URL) and checks it strictly.
    inline Link parse (const juce::String& commandLine)
    {
        Link link;
        for (auto token : juce::StringArray::fromTokens (commandLine, true))
        {
            token = token.unquoted().trim();
            if (! token.startsWithIgnoreCase (juce::String (kScheme) + "://"))
                continue;

            auto rest = token.fromFirstOccurrenceOf ("://", false, false)
                             .upToFirstOccurrenceOf ("?", false, false)
                             .upToFirstOccurrenceOf ("#", false, false);
            while (rest.endsWithChar ('/'))
                rest = rest.dropLastCharacters (1);

            const auto action = rest.upToFirstOccurrenceOf ("/", false, false).toLowerCase();
            const auto id     = rest.fromFirstOccurrenceOf ("/", false, false);

            const bool idOk = id.isNotEmpty() && id.length() <= 64
                           && id.containsOnly ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");

            if ((action == "install" || action == "plugin") && idOk)
                return { action, id };
            if (action == "updates" || action == "open" || action.isEmpty())
                return { action.isEmpty() ? juce::String ("open") : action, {} };
        }
        return link;
    }

    // Windows: HKCU\Software\Classes\ronecenter -> this exe. Rewritten only when it
    // does not already point here (an update may have moved the executable).
    inline void registerScheme()
    {
       #if JUCE_WINDOWS
        if (! AutoStart::isInstalledCopy())
            return;   // a build from a source tree never takes the link over (AutoStart.h)
        const auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getFullPathName();
        const auto command = "\"" + exe + "\" " + kFlag + " \"%1\"";
        const juce::String root = "HKEY_CURRENT_USER\\Software\\Classes\\" + juce::String (kScheme) + "\\";

        if (juce::WindowsRegistry::getValue (root + "shell\\open\\command\\") == command)
            return;

        juce::WindowsRegistry::setValue (root, "URL:RONE Plugins Center");
        juce::WindowsRegistry::setValue (root + "URL Protocol", juce::String());
        juce::WindowsRegistry::setValue (root + "DefaultIcon\\", "\"" + exe + "\",0");
        juce::WindowsRegistry::setValue (root + "shell\\open\\command\\", command);
       #endif
    }
}

// ============================================================================
// CenterSettings - the few choices the Center's backend acts on (the page
// keeps its own look-and-feel choices in its browser storage).
// ============================================================================
namespace CenterSettings
{
    inline juce::File file()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("RonePluginsCenter").getChildFile ("settings.json");
    }

    inline juce::var load()
    {
        auto v = juce::JSON::parse (file().loadFileAsString());
        return v.isObject() ? v : juce::var (new juce::DynamicObject());
    }

    inline bool getBool (const juce::Identifier& key, bool fallback)
    {
        const auto v = load();
        return v.hasProperty (key) ? (bool) v.getProperty (key, fallback) : fallback;
    }

    inline void setBool (const juce::Identifier& key, bool value)
    {
        auto v = load();
        if (auto* o = v.getDynamicObject())
            o->setProperty (key, value);
        file().getParentDirectory().createDirectory();
        file().replaceWithText (juce::JSON::toString (v));
    }

    // Download updates in the background so Update installs at once. On by default.
    static const juce::Identifier backgroundDownload ("backgroundDownload");
}
