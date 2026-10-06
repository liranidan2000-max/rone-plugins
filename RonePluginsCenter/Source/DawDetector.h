#pragma once

#include <JuceHeader.h>

// ============================================================================
// DawDetector - which DAWs are on this computer (Center 2.0).
//
// "I installed it and the plugin is not in my DAW" is the support question
// that comes back most: the DAW keeps the list it scanned and only learns about
// a new plugin when it is told to look again. Knowing the DAW lets the page
// show the right steps after every install ("FL Studio: Options > Manage
// plugins > Start scan") instead of a generic "rescan your plugins".
//
// Read from the folders the DAWs install into - cheap, no process scan, no
// registry walk on a timer. The newest version of each DAW is the one named.
// ============================================================================
namespace DawDetector
{
    struct Daw
    {
        juce::String id;      // fl | ableton | cubase | studioone | reaper | bitwig | logic
        juce::String name;    // "FL Studio 2026", "Ableton Live 12 Suite"
    };

    namespace detail
    {
        // Newest first by the number in the name ("FL Studio 2026" over "FL Studio 21").
        inline juce::String newest (juce::StringArray names)
        {
            names.sortNatural();
            return names.isEmpty() ? juce::String() : names[names.size() - 1];
        }

        inline juce::StringArray childDirs (const juce::File& dir, const juce::String& prefix,
                                            const juce::String& mustContain = {})
        {
            juce::StringArray out;
            if (! dir.isDirectory())
                return out;
            for (const auto& f : dir.findChildFiles (juce::File::findDirectories, false, prefix + "*"))
                if (mustContain.isEmpty() || f.getChildFile (mustContain).exists())
                    out.add (f.getFileName());
            return out;
        }
    }

    inline juce::Array<Daw> find()
    {
        juce::Array<Daw> found;
        auto add = [&found] (const char* id, const juce::String& name)
        {
            if (name.isNotEmpty())
                found.add ({ id, name.trim() });
        };

       #if JUCE_WINDOWS
        const auto pf = juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory);     // C:\Program Files
        const auto pd = juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory);  // C:\ProgramData

        add ("fl", detail::newest (detail::childDirs (pf.getChildFile ("Image-Line"), "FL Studio ", "FL64.exe")));

        // Ableton keeps one folder per edition under ProgramData: "Live 12 Suite"
        {
            const auto live = detail::newest (detail::childDirs (pd.getChildFile ("Ableton"), "Live "));
            if (live.isNotEmpty())
                add ("ableton", "Ableton " + live);
        }

        add ("cubase",    detail::newest (detail::childDirs (pf.getChildFile ("Steinberg"), "Cubase")));
        add ("studioone", detail::newest (detail::childDirs (pf.getChildFile ("PreSonus"), "Studio One")));

        if (pf.getChildFile ("REAPER (x64)").getChildFile ("reaper.exe").existsAsFile()
            || pf.getChildFile ("REAPER").getChildFile ("reaper.exe").existsAsFile())
            add ("reaper", "REAPER");

        if (pf.getChildFile ("Bitwig Studio").isDirectory())
            add ("bitwig", "Bitwig Studio");

       #elif JUCE_MAC
        const juce::File apps ("/Applications");
        auto appsNamed = [&apps] (const juce::String& prefix)
        {
            juce::StringArray out;
            for (const auto& f : apps.findChildFiles (juce::File::findDirectories, false, prefix + "*.app"))
                out.add (f.getFileNameWithoutExtension());
            return out;
        };

        add ("fl",        detail::newest (appsNamed ("FL Studio")));
        add ("ableton",   detail::newest (appsNamed ("Ableton Live")));
        add ("logic",     detail::newest (appsNamed ("Logic Pro")));
        add ("cubase",    detail::newest (appsNamed ("Cubase")));
        add ("studioone", detail::newest (appsNamed ("Studio One")));
        add ("reaper",    detail::newest (appsNamed ("REAPER")));
        add ("bitwig",    detail::newest (appsNamed ("Bitwig Studio")));
       #endif

        return found;
    }

    inline juce::var toVar (const juce::Array<Daw>& daws)
    {
        juce::Array<juce::var> arr;
        for (const auto& d : daws)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("id", d.id);
            o->setProperty ("name", d.name);
            arr.add (juce::var (o));
        }
        return arr;
    }
}
