#pragma once

// Shared/RoneExportFolder.h - where RONE plugins put the audio they create.
//
// Clips that leave a plugin by DRAG TO EXPORT (or an EXPORT button) used to be
// written to the system temp folder. A DAW references the dropped file by path,
// Windows cleans temp up, and the project later says "sample not found". So
// every export now lands in a permanent, user-visible folder:
//
//     <Documents>/RONE Plugins/Exports/<Product>/<name>_<date>_<time>.<ext>
//
// and is never deleted by the plugin.

#include <juce_core/juce_core.h>

namespace RoneExport
{

inline juce::File getExportsDir (const juce::String& productName)
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("RONE Plugins")
                   .getChildFile ("Exports")
                   .getChildFile (productName);

    if (! dir.exists())
        dir.createDirectory();

    return dir;
}

/** A file that does not exist yet: <baseName>_<YYYY-MM-DD_HHMMSS><extension>,
    with a counter appended if two exports land in the same second. */
inline juce::File nextExportFile (const juce::String& productName,
                                  const juce::String& baseName,
                                  const juce::String& extension)
{
    auto dir   = getExportsDir (productName);
    auto ext   = extension.startsWithChar ('.') ? extension : "." + extension;
    auto stamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H%M%S");
    auto safe  = juce::File::createLegalFileName (baseName.isEmpty() ? "Export" : baseName);

    auto file = dir.getChildFile (safe + "_" + stamp + ext);

    for (int n = 2; file.exists(); ++n)
        file = dir.getChildFile (safe + "_" + stamp + "_" + juce::String (n) + ext);

    return file;
}

} // namespace RoneExport
