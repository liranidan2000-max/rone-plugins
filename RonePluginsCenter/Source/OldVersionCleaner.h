#pragma once

#include <JuceHeader.h>
#include "VersionChecker.h"
#include "PluginInUse.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <shellapi.h>
 #pragma comment (lib, "shell32.lib")
#endif

// ============================================================================
// OldVersionCleaner - Settings > DELETE OLD VERSIONS.
//
// Finds files that earlier RONE installs left behind and that make a DAW load
// the wrong thing, and deletes them. Liran, 2026-09-26, after RONE Reverse
// Reverb 1.1.7 kept showing up in FL Studio as a synth: the bundle still held a
// Contents/Resources/moduleinfo.json from a 2026-09-02 build that said
// "Instrument|Synth", and FL believes that file over the DLL. Every plugin now
// builds without that manifest, so no installer ever replaced or removed it.
//
// What counts (and only this - it never deletes the only copy of a plugin):
//   manifest   Contents/Resources/moduleinfo.json older than the bundle's own
//              binary: a description of a previous build.
//   duplicate  a bundle in the top VST3 folder that also sits in VST3\RONE
//              (an old dev build or the pre-RONE-folder install); DAWs load the
//              stale top-level copy (see the 2026-09-16 Throw incident).
//   legacy     a pre-2026-09-02 name (ReverseReverb.vst3, RONE Bridge.vst3, ...)
//              while the current name is installed too. Windows and macOS file
//              names ignore case, so "Rone Flanger" vs "RONE Flanger" in one
//              folder is the same file and is never touched.
//   leftover   a quarantined copy from an update that met a locked file:
//              *.locked-old*, *.shadow-old*, _RONE_shadow_quarantine_*.
//   backup     a whole older copy kept beside the current one under another
//              name: "RONE Iron.vst3.beta1-20261108", "RONE Iron.vst3.bak",
//              "RONE Iron.vst3.r6-1759" ... A DAW scans INTO such a folder,
//              finds a complete plugin and lists it again - FL Studio showed
//              RONE Iron, RONE Iron_2 ... RONE Iron_13 (Liran, 2026-11-16, ~100
//              of them). Only while "RONE Iron.vst3" itself is there.
//   daw-list   an FL Studio plugin-list entry (Plugin database/Installed/.../
//              <name>_N.fst) that points at a RONE file that is gone, or at one
//              of the copies above. FL never forgets them on its own: they are
//              the _2, _3 ... rows of its plugin picker. Removed only after the
//              file it points at is really gone, and never an entry that names
//              no file (FL finds those by the plugin's name - the current one).
//
// The user picks what goes: every row has a tick box (all ticked); the delete
// gets the ticked paths and deletes only rows its own fresh scan finds too.
//
// Deleting needs admin rights on most machines (Program Files, /Library), so
// the Center first tries as the user and then runs itself once, elevated, with
// --delete-old-versions; that instance scans again with the same rules and
// deletes only what it finds itself (no paths are passed to it). Anything a
// DAW still has loaded is skipped and reported by the DAW's name.
//
// --test-root "<dir>" (command line only, with --list/--delete): the roots become
// sub-folders of that folder - how the rules are tested on a fake tree. The
// elevated run is started with --delete-old-versions alone, so it cannot be steered.
// ============================================================================
namespace OldVersionCleaner
{
    constexpr const char* kDeleteFlag = "--delete-old-versions";
    constexpr const char* kListFlag   = "--list-old-versions";

    struct Roots
    {
        juce::File roneVst3;     // where the installers put bundles (Windows: ...\VST3\RONE)
        juce::File topVst3;      // Windows: ...\VST3 (the top-level folder); macOS: the same folder
        juce::File apps;         // standalone apps (Windows: Program Files\RONE Plugins)
        juce::File appsAlt;      // macOS: /Applications/RONE Plugins
        juce::File au;           // macOS: Components
    };

    inline Roots testRoots (const juce::File& base)
    {
        Roots r;
        r.topVst3  = base.getChildFile ("VST3");
        r.roneVst3 = r.topVst3.getChildFile ("RONE");
        r.apps     = base.getChildFile ("Apps");
        r.au       = base.getChildFile ("Components");
        return r;
    }

    inline Roots defaultRoots()
    {
        Roots r;
        r.roneVst3 = VersionChecker::getVst3InstallDir();
       #if JUCE_WINDOWS
        r.topVst3 = r.roneVst3.getParentDirectory();
        r.apps    = VersionChecker::getStandaloneInstallDir();
       #elif JUCE_MAC
        r.topVst3 = r.roneVst3;                          // no RONE sub-folder on macOS
        r.apps    = juce::File ("/Applications");
        r.appsAlt = juce::File ("/Applications/RONE Plugins");
        r.au      = VersionChecker::getAUInstallDir();
       #else
        r.topVst3 = r.roneVst3.getParentDirectory();
       #endif
        return r;
    }

    struct Item
    {
        juce::File file;
        juce::String kind;       // manifest | duplicate | legacy | leftover | backup | daw-list
        juce::String reason;     // one sentence for the confirm list
        juce::String status;     // after delete: deleted | in_use | failed
        juce::StringArray hosts; // in_use: who holds it ("FL Studio")
        juce::String plugin;     // the product it belongs to ("RONE Iron") - the list groups by it
        juce::String version;    // the old copy's version, when it has one
        juce::File target;       // daw-list: the plugin file the entry points at
    };

    // The current copy of one product, shown as "keeps 1.2.1" above its old copies.
    struct Kept { juce::String plugin, version; juce::File file; };

    // Old name -> the name that replaced it (installer/*.iss [InstallDelete], 2026-09).
    // Names that differ only in case are the same file and are skipped below.
    inline juce::StringPairArray legacyNames()
    {
        juce::StringPairArray m;
        m.set ("ReverseReverb",  "RONE Reverse Reverb");
        m.set ("RONE Bridge",    "RONE Analyzer Bridge");
        m.set ("Rone Flanger",   "RONE Flanger");
        m.set ("Rone Stucker",   "RONE Stucker");
        m.set ("Rone Stutter",   "RONE Stutter");
        m.set ("Rone Sync Verb", "RONE Sync Verb");
        return m;
    }

    inline bool isRoneName (const juce::String& fileName)
    {
        return fileName.startsWith ("RONE ") || fileName.startsWith ("Rone ") || fileName.startsWithIgnoreCase ("ReverseReverb");
    }

    // RONE's own plus the synth that carries no "RONE" in its name.
    inline bool isOurs (const juce::String& fileName)
    {
        return isRoneName (fileName) || fileName.startsWithIgnoreCase ("KALIFONIC");
    }

    // The exact entry in `dir` whose name matches `name` ignoring case, or {}.
    inline juce::File findEntry (const juce::File& dir, const juce::String& name)
    {
        if (! dir.isDirectory()) return {};
        for (const auto& e : juce::RangedDirectoryIterator (dir, false, "*", juce::File::findFilesAndDirectories))
            if (e.getFile().getFileName().equalsIgnoreCase (name))
                return e.getFile();
        return {};
    }

    inline juce::File bundleBinary (const juce::File& bundle)
    {
        const auto base = bundle.getFileNameWithoutExtension();
        auto win = bundle.getChildFile ("Contents/x86_64-win").getChildFile (base + ".vst3");
        if (win.existsAsFile()) return win;
        auto mac = bundle.getChildFile ("Contents/MacOS").getChildFile (base);
        if (mac.existsAsFile()) return mac;
        return {};
    }

    inline juce::String manifestVersion (const juce::File& json)
    {
        const auto parsed = juce::JSON::parse (json.loadFileAsString());
        if (auto* classes = parsed.getProperty ("Classes", {}).getArray())
            for (const auto& c : *classes)
                if (c.getProperty ("Version", {}).toString().isNotEmpty())
                    return c.getProperty ("Version", {}).toString();
        // The files VST3 SDK writes carry trailing commas that JUCE's parser refuses.
        const auto text = json.loadFileAsString();
        const auto at = text.indexOf ("\"Version\"");
        if (at < 0) return {};
        return text.substring (at).fromFirstOccurrenceOf (":", false, false)
                   .fromFirstOccurrenceOf ("\"", false, false).upToFirstOccurrenceOf ("\"", false, false);
    }

    // Version of a binary (Windows: its version resource) or of a bundle / app
    // (macOS: Contents/Info.plist). Empty when there is none.
    inline juce::String fileVersion (const juce::File& f)
    {
        if (f == juce::File() || ! f.exists()) return {};
       #if JUCE_WINDOWS
        auto v = f.existsAsFile() ? f.getVersion() : juce::String();
        if (v.endsWith (".0") && juce::StringArray::fromTokens (v, ".", {}).size() == 4)
            v = v.dropLastCharacters (2);                       // "1.2.1.0" -> "1.2.1"
        return v;
       #else
        const auto plist = f.getChildFile ("Contents/Info.plist").loadFileAsString();
        const auto at = plist.indexOf ("CFBundleShortVersionString");
        if (at < 0) return {};
        return plist.substring (at).fromFirstOccurrenceOf ("<string>", false, false).upToFirstOccurrenceOf ("</string>", false, false).trim();
       #endif
    }

    // "RONE Iron.vst3.beta3-20261108" -> its version. A backup is either the bundle
    // renamed (Contents directly inside) or a folder that holds the bundle.
    inline juce::String versionOfCopy (const juce::File& copy, const juce::String& base, const juce::String& ext)
    {
        if (copy.existsAsFile()) return fileVersion (copy);
       #if JUCE_WINDOWS
        if (ext == "vst3")
        {
            for (const auto& b : { copy, copy.getChildFile (base + ".vst3") })
            {
                const auto bin = b.getChildFile ("Contents/x86_64-win").getChildFile (base + ".vst3");
                if (bin.existsAsFile()) return fileVersion (bin);
            }
        }
        return {};
       #else
        auto v = fileVersion (copy);
        return v.isNotEmpty() ? v : fileVersion (copy.getChildFile (base + "." + ext));
       #endif
    }

    inline juce::String versionOfCurrent (const juce::File& current)
    {
       #if JUCE_WINDOWS
        if (current.isDirectory())
            return fileVersion (current.getChildFile ("Contents/x86_64-win").getChildFile (current.getFileName()));
       #endif
        return fileVersion (current);
    }

    // ---- FL Studio's plugin list ------------------------------------------------
    // Every plugin FL found is one small .fst in <Shared data>/FL Studio/Presets/
    // Plugin database/Installed/{Effects,Generators}/{VST,VST3}. Inside, a run of
    // records [u32 id][u64 size][data]; id 55 holds the plugin file's full path.
    inline juce::File flSharedData()
    {
       #if JUCE_WINDOWS
        const auto reg = juce::WindowsRegistry::getValue ("HKEY_CURRENT_USER\\Software\\Image-Line\\Shared\\Paths\\Shared data");
        if (reg.isNotEmpty() && juce::File::isAbsolutePath (reg) && juce::File (reg).isDirectory())
            return juce::File (reg);
       #endif
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Image-Line");
    }

    // The plugin path an .fst points at, or "" when it names none.
    inline juce::String flEntryPath (const juce::File& fst)
    {
        juce::MemoryBlock mb;
        if (fst.getSize() > 65536 || ! fst.loadFileAsData (mb)) return {};
        const auto* b = static_cast<const juce::uint8*> (mb.getData());
        const size_t n = mb.getSize();
        auto u32 = [b] (size_t at) { return (juce::uint32) b[at] | ((juce::uint32) b[at + 1] << 8)
                                          | ((juce::uint32) b[at + 2] << 16) | ((juce::uint32) b[at + 3] << 24); };
        for (size_t i = 0; i + 12 < n; ++i)
        {
            if (u32 (i) != 55u || u32 (i + 8) != 0u) continue;
            const auto size = (size_t) u32 (i + 4);
            if (size < 4 || size > 2048 || i + 12 + size > n) continue;
            const auto path = juce::String::fromUTF8 (reinterpret_cast<const char*> (b + i + 12), (int) size);
            if (juce::File::isAbsolutePath (path)) return path;
        }
        return {};
    }

    // "RONE Iron_12" -> "RONE Iron" (FL numbers the second, third ... row of one name)
    inline juce::String flBaseName (const juce::String& entryName)
    {
        const int us = entryName.lastIndexOfChar ('_');
        if (us > 0 && entryName.length() > us + 1 && entryName.substring (us + 1).containsOnly ("0123456789"))
            return entryName.substring (0, us);
        return entryName;
    }

    // FL's row file is an FLhd header plus one FLdt chunk of events (id < 64: 1 byte,
    // < 128: 2, < 192: 4, else a varint length and data). Event 203 is the row's
    // name, UTF-16. Writes "<newName>.fst" beside it with that name and removes the
    // old file; anything that does not parse exactly is left as it is.
    inline bool flRenameEntry (const juce::File& fst, const juce::String& newName)
    {
        const auto dest = fst.getSiblingFile (newName + ".fst");
        if (dest.exists()) return false;
        juce::MemoryBlock mb;
        if (fst.getSize() > 65536 || ! fst.loadFileAsData (mb)) return false;
        const auto* b = static_cast<const juce::uint8*> (mb.getData());
        const size_t n = mb.getSize();
        auto u32 = [b] (size_t at) { return (size_t) b[at] | ((size_t) b[at + 1] << 8) | ((size_t) b[at + 2] << 16) | ((size_t) b[at + 3] << 24); };
        if (n < 22 || std::memcmp (b, "FLhd", 4) != 0 || u32 (4) != 6 || std::memcmp (b + 14, "FLdt", 4) != 0) return false;
        const size_t dataLenAt = 18, start = 22;
        if (start + u32 (dataLenAt) != n) return false;

        size_t i = start, nameAt = 0, nameEnd = 0;
        while (i < n)
        {
            const size_t evAt = i;
            const auto id = b[i++];
            if (id < 64)       i += 1;
            else if (id < 128) i += 2;
            else if (id < 192) i += 4;
            else
            {
                size_t len = 0; int shift = 0;
                while (i < n && shift < 28) { const auto c = b[i++]; len |= (size_t) (c & 0x7F) << shift; shift += 7; if ((c & 0x80) == 0) break; }
                i += len;
                if (id == 203) { nameAt = evAt; nameEnd = i; }
            }
        }
        if (i != n || nameAt == 0) return false;

        juce::MemoryBlock name;                                   // UTF-16LE + terminating 0
        for (auto* c = newName.toUTF16().getAddress(); *c != 0; ++c) { const juce::uint8 two[] { (juce::uint8) (*c & 0xFF), (juce::uint8) (*c >> 8) }; name.append (two, 2); }
        const juce::uint8 zero[] { 0, 0 }; name.append (zero, 2);

        juce::MemoryBlock out;
        out.append (b, nameAt);
        juce::uint8 head[6]; size_t h = 0; head[h++] = 203;
        for (size_t len = name.getSize();; ) { const auto c = (juce::uint8) (len & 0x7F); len >>= 7; head[h++] = (juce::uint8) (c | (len ? 0x80 : 0)); if (! len) break; }
        out.append (head, h);
        out.append (name.getData(), name.getSize());
        out.append (b + nameEnd, n - nameEnd);
        const auto dataLen = (juce::uint32) (out.getSize() - start);
        auto* o = static_cast<juce::uint8*> (out.getData());
        for (int k = 0; k < 4; ++k) o[dataLenAt + (size_t) k] = (juce::uint8) ((dataLen >> (8 * k)) & 0xFF);

        if (! dest.replaceWithData (out.getData(), out.getSize())) return false;
        if (flEntryPath (dest) != flEntryPath (fst)) { dest.deleteFile(); return false; }
        fst.deleteFile();
        return true;
    }

    // After its old rows went: the one row left for a plugin in a folder gets its plain
    // name back ("RONE 1176_2" -> "RONE 1176") when nothing else has it.
    inline void flTidyNames (const juce::Array<juce::File>& dirs)
    {
        for (const auto& dir : dirs)
        {
            juce::StringPairArray onlyRow;   // base -> the single numbered row ("" when more than one)
            juce::StringArray plain;
            for (const auto& e : juce::RangedDirectoryIterator (dir, false, "*.fst", juce::File::findFiles))
            {
                const auto entry = e.getFile().getFileNameWithoutExtension();
                if (! isOurs (entry)) continue;
                const auto base = flBaseName (entry);
                if (base == entry) { plain.add (base.toLowerCase()); continue; }
                const auto key = base.toLowerCase();
                onlyRow.set (key, onlyRow.containsKey (key) ? juce::String() : entry);
            }
            for (const auto& key : onlyRow.getAllKeys())
            {
                const auto entry = onlyRow[key];
                if (entry.isEmpty() || plain.contains (key)) continue;
                const auto fst = dir.getChildFile (entry + ".fst");
                const auto path = flEntryPath (fst);
                if (path.isEmpty() || ! juce::File (path).exists()) continue;
                flRenameEntry (fst, flBaseName (entry));
            }
        }
    }

    // The product a plugin file belongs to: ".../RONE Iron.vst3.beta1/Contents/x86_64-win/RONE Iron.vst3" -> "RONE Iron"
    inline juce::String productOf (const juce::File& f)
    {
        return f.getFileName().upToFirstOccurrenceOf (".", false, false);
    }

    inline juce::Array<Item> scan (const Roots& r = defaultRoots(), juce::Array<Kept>* kept = nullptr,
                                   const juce::File& flData = flSharedData())
    {
        juce::Array<Item> items;
        auto add = [&items] (const juce::File& f, const juce::String& kind, const juce::String& reason,
                             const juce::String& plugin = {}, const juce::String& version = {}, const juce::File& target = {})
        {
            for (const auto& i : items)
                if (i.file == f || f.isAChildOf (i.file)) return;   // already going, with its folder
            Item it { f, kind, reason, {}, {}, plugin, version, target };
            if (it.plugin.isEmpty()) it.plugin = productOf (f);
            items.add (it);
        };

        const bool separateTop = r.topVst3 != r.roneVst3;
        const auto legacy = legacyNames();

        // leftover: quarantined copies from updates that met a locked file
        for (const auto& dir : { r.roneVst3, r.topVst3 })
        {
            if (! dir.isDirectory()) continue;
            for (const auto& e : juce::RangedDirectoryIterator (dir, false, "*", juce::File::findFilesAndDirectories))
            {
                const auto f = e.getFile();
                const auto n = f.getFileName();
                if ((isRoneName (n) && (n.containsIgnoreCase (".locked-old") || n.containsIgnoreCase (".shadow-old")))
                    || n.startsWithIgnoreCase ("_RONE_shadow_quarantine_"))
                    add (f, "leftover", "A copy an earlier update set aside because a program was using it");
            }
        }

        // backup: "<name>.vst3.<anything>" (or .component / .app / .exe) beside "<name>.vst3"
        {
            struct Place { juce::File dir; juce::String ext; };
            const Place places[] { { r.roneVst3, "vst3" }, { separateTop ? r.topVst3 : juce::File(), "vst3" },
                                   { r.au, "component" }, { r.apps, "app" }, { r.appsAlt, "app" }, { r.apps, "exe" } };
            for (const auto& p : places)
            {
                if (p.dir == juce::File() || ! p.dir.isDirectory()) continue;
                const auto marker = "." + p.ext + ".";
                for (const auto& e : juce::RangedDirectoryIterator (p.dir, false, "*", juce::File::findFilesAndDirectories))
                {
                    const auto f = e.getFile();
                    const auto n = f.getFileName();
                    const int at = n.indexOfIgnoreCase (marker);
                    if (at <= 0 || ! isOurs (n)) continue;
                    const auto base = n.substring (0, at);
                    const auto current = findEntry (p.dir, base + "." + p.ext);
                    if (current == juce::File() || current == f) continue;     // never the only copy
                    const auto v = versionOfCopy (f, base, p.ext);
                    add (f, "backup", "An older copy of " + current.getFileName() + (v.isNotEmpty() ? " (version " + v + ")" : juce::String())
                                      + " - your DAW lists it as one more " + base, base, v);
                }
            }
        }

        // duplicate: a bundle in the top VST3 folder that the RONE folder has too
        if (separateTop && r.topVst3.isDirectory() && r.roneVst3.isDirectory())
        {
            for (const auto& e : juce::RangedDirectoryIterator (r.topVst3, false, "*.vst3", juce::File::findDirectories))
            {
                const auto f = e.getFile();
                const auto base = f.getFileNameWithoutExtension();
                if (! isRoneName (f.getFileName())) continue;
                // RONE Analyzer's installer puts its Bridge HERE, at the top: that copy is the real one
                if (base.equalsIgnoreCase ("RONE Analyzer Bridge") || base.equalsIgnoreCase ("RONE Bridge")) continue;

                auto current = findEntry (r.roneVst3, f.getFileName());
                if (current == juce::File() && legacy.containsKey (base))
                    current = findEntry (r.roneVst3, legacy[base] + ".vst3");
                if (current != juce::File() && current.isDirectory())
                    add (f, "duplicate", "A second copy outside the RONE folder - your DAW may load this old one instead of " + current.getFileName());
            }
        }

        // legacy: an old name next to its current name (never when they are the same file)
        const auto legacyKeys = legacy.getAllKeys();
        for (const auto& oldBase : legacyKeys)
        {
            const auto newBase = legacy[oldBase];
            if (oldBase.equalsIgnoreCase (newBase)) continue;             // same file on disk

            struct Place { juce::File dir; juce::String ext; };
            const Place places[] { { r.roneVst3, ".vst3" }, { separateTop ? r.topVst3 : juce::File(), ".vst3" },
                                   { r.apps, ".exe" }, { r.apps, ".app" }, { r.appsAlt, ".app" }, { r.au, ".component" } };
            for (const auto& p : places)
            {
                if (p.dir == juce::File() || ! p.dir.isDirectory()) continue;
                const auto oldOne = findEntry (p.dir, oldBase + p.ext);
                const auto newOne = findEntry (p.dir, newBase + p.ext);
                if (oldOne != juce::File() && newOne != juce::File() && oldOne != newOne)
                    add (oldOne, "legacy", "The old name of " + newOne.getFileName() + ", which is installed next to it");
            }
        }

        // manifest: a moduleinfo.json older than the bundle's binary
        juce::Array<juce::File> vst3Dirs { r.roneVst3 };
        if (separateTop) vst3Dirs.add (r.topVst3);
        for (const auto& dir : vst3Dirs)
        {
            if (! dir.isDirectory()) continue;
            for (const auto& e : juce::RangedDirectoryIterator (dir, false, "*.vst3", juce::File::findDirectories))
            {
                const auto bundle = e.getFile();
                if (! isRoneName (bundle.getFileName())) continue;
                const auto json = bundle.getChildFile ("Contents/Resources/moduleinfo.json");
                const auto bin = bundleBinary (bundle);
                if (! json.existsAsFile() || bin == juce::File()) continue;

                const auto age = bin.getLastModificationTime() - json.getLastModificationTime();
                if (age.inMinutes() > 10.0)
                {
                    const auto v = manifestVersion (json);
                    add (json, "manifest", "An old description file" + (v.isNotEmpty() ? " (version " + v + ")" : juce::String())
                                         + " inside " + bundle.getFileName() + " - your DAW reads it instead of the plugin");
                }
            }
        }


        // daw-list: FL Studio rows that point at a RONE file that is gone, or that goes now
        const auto flInstalled = flData.getChildFile ("FL Studio/Presets/Plugin database/Installed");
        if (flInstalled.isDirectory())
        {
            const int filesBefore = items.size();
            auto goingNow = [&items, filesBefore] (const juce::File& t)
            {
                for (int i = 0; i < filesBefore; ++i)
                    if (t == items.getReference (i).file || t.isAChildOf (items.getReference (i).file)) return true;
                return false;
            };
            for (const auto& e : juce::RangedDirectoryIterator (flInstalled, true, "*.fst", juce::File::findFiles))
            {
                const auto fst = e.getFile();
                const auto entry = fst.getFileNameWithoutExtension();
                if (! isOurs (entry)) continue;
                const auto path = flEntryPath (fst);
                if (path.isEmpty()) continue;                        // FL finds it by name: the current one
                const juce::File target (path);
                if (! isOurs (target.getFileName())) continue;       // only rows that load a RONE file
                const bool gone = ! target.exists();
                if (! gone && ! goingNow (target)) continue;
                const auto where = fst.getParentDirectory().getParentDirectory().getFileName();   // Effects | Generators
                add (fst, "daw-list", "FL Studio row \"" + entry + "\" (" + where + ") - "
                                      + (gone ? "points at a copy that is no longer there" : "points at an old copy being deleted"),
                     flBaseName (entry), {}, target);
            }
        }

        // kept: the current copy of every product that has something old
        if (kept != nullptr)
        {
            juce::StringArray seen;
            for (const auto& it : items)
            {
                if (seen.contains (it.plugin)) continue;
                seen.add (it.plugin);
                juce::File cur;
                for (const auto& d : { r.roneVst3, r.au, r.apps, r.appsAlt })
                {
                    if (d == juce::File() || ! d.isDirectory()) continue;
                    for (const auto* ext : { ".vst3", ".component", ".app", ".exe" })
                        if ((cur = findEntry (d, it.plugin + ext)) != juce::File()) break;
                    if (cur != juce::File()) break;
                }
                if (cur != juce::File())
                    kept->add ({ it.plugin, versionOfCurrent (cur), cur });
            }
        }

        return items;
    }

    inline bool samePath (const juce::File& a, const juce::String& b)
    {
       #if JUCE_WINDOWS
        return a.getFullPathName().equalsIgnoreCase (juce::File (b).getFullPathName());
       #else
        return a.getFullPathName() == juce::File (b).getFullPathName();
       #endif
    }

    // Only the rows the user left ticked. An empty `only` keeps everything (the old
    // behaviour, and the command line without --only-file).
    inline void keepOnly (juce::Array<Item>& items, const juce::StringArray& only)
    {
        if (only.isEmpty()) return;
        for (int i = items.size(); --i >= 0;)
        {
            bool picked = false;
            for (const auto& p : only)
                if (samePath (items.getReference (i).file, p)) { picked = true; break; }
            if (! picked) items.remove (i);
        }
    }

    inline void deleteOne (Item& it)
    {
        if (! it.file.exists()) { it.status = "deleted"; return; }

       #if JUCE_WINDOWS
        if (it.kind != "daw-list")
        {
            const bool isApp = it.file.hasFileExtension ("exe");
            auto holders = PluginInUse::find (isApp ? juce::File() : it.file, isApp ? it.file : juce::File());
            if (! holders.isEmpty())
            {
                it.status = "in_use";
                it.hosts = holders.hosts;
                if (holders.ownApp) it.hosts.addIfNotAlreadyThere (it.file.getFileNameWithoutExtension());
                return;
            }
        }
       #endif

        if (it.file.isDirectory()) it.file.deleteRecursively (false);
        else                       it.file.deleteFile();
        it.status = it.file.exists() ? "failed" : "deleted";
    }

    // Deletes as the current user. Files first; an FL row only once the file it
    // points at is really gone (a copy a DAW still holds keeps its row).
    // In-use items are skipped; failures stay "failed".
    inline void deleteItems (juce::Array<Item>& items, bool includeDawLists = true)
    {
        for (auto& it : items)
            if (it.kind != "daw-list") deleteOne (it);

        if (! includeDawLists) return;
        juce::Array<juce::File> dirs;
        for (auto& it : items)
        {
            if (it.kind != "daw-list") continue;
            if (it.target != juce::File() && it.target.exists()) { it.status = "kept"; continue; }
            deleteOne (it);
            if (it.status == "deleted") dirs.addIfNotAlreadyThere (it.file.getParentDirectory());
        }
        flTidyNames (dirs);
    }

    // The same program, once, with admin rights and --delete-old-versions; waits for it.
    // `onlyFile` lists the ticked paths: the elevated scan deletes only rows that are in
    // it (it can narrow what goes, never widen it). Returns false when the user declined
    // or the OS could not start it.
    inline bool runElevated (const juce::File& onlyFile)
    {
        const auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
       #if JUCE_WINDOWS
        const auto file = exe.getFullPathName();
        juce::String params (kDeleteFlag);
        if (onlyFile != juce::File()) params << " --only-file \"" << onlyFile.getFullPathName() << "\"";
        SHELLEXECUTEINFOW info {};
        info.cbSize = sizeof (info);
        info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
        info.lpVerb = L"runas";
        info.lpFile = file.toWideCharPointer();
        info.lpParameters = params.toWideCharPointer();
        info.nShow = SW_HIDE;
        if (! ShellExecuteExW (&info) || info.hProcess == nullptr)
            return false;
        WaitForSingleObject (info.hProcess, 300000);
        CloseHandle (info.hProcess);
        return true;
       #elif JUCE_MAC
        auto esc = [] (const juce::String& s) { return s.replace ("\\", "\\\\").replace ("\"", "\\\""); };
        juce::String script = "do shell script quoted form of \"" + esc (exe.getFullPathName()) + "\" & \" " + juce::String (kDeleteFlag);
        if (onlyFile != juce::File())
            script << " --only-file \" & quoted form of \"" << esc (onlyFile.getFullPathName()) << "\"";
        else
            script << "\"";
        script << " with administrator privileges";
        juce::StringArray cmd { "/usr/bin/osascript", "-e", script };
        juce::ChildProcess p;
        if (! p.start (cmd)) return false;
        p.waitForProcessToFinish (300000);
        return p.getExitCode() == 0;
       #else
        juce::ignoreUnused (exe, onlyFile);
        return false;
       #endif
    }

    inline juce::var toVar (const Item& it)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("path", it.file.getFullPathName());
        o->setProperty ("name", it.kind == "manifest" ? it.file.getParentDirectory().getParentDirectory().getParentDirectory().getFileName()
                                                         + " / moduleinfo.json"
                                                       : it.file.getFileName());
        o->setProperty ("kind", it.kind);
        o->setProperty ("reason", it.reason);
        o->setProperty ("plugin", it.plugin);
        if (it.version.isNotEmpty()) o->setProperty ("version", it.version);
        if (it.target != juce::File()) o->setProperty ("target", it.target.getFullPathName());
        if (it.status.isNotEmpty()) o->setProperty ("status", it.status);
        if (! it.hosts.isEmpty()) o->setProperty ("hosts", it.hosts.joinIntoString (", "));
        return juce::var (o);
    }

    inline juce::var toVar (const juce::Array<Item>& items)
    {
        juce::Array<juce::var> arr;
        for (const auto& it : items) arr.add (toVar (it));
        return juce::var (arr);
    }

    inline juce::var toVar (const juce::Array<Kept>& kept)
    {
        juce::Array<juce::var> arr;
        for (const auto& k : kept)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("plugin", k.plugin);
            o->setProperty ("version", k.version);
            o->setProperty ("path", k.file.getFullPathName());
            arr.add (juce::var (o));
        }
        return juce::var (arr);
    }

    // The scan the list shows: { items, kept }.
    inline juce::var scanForList()
    {
        juce::Array<Kept> kept;
        const auto items = scan (defaultRoots(), &kept);
        auto* o = new juce::DynamicObject();
        o->setProperty ("success", true);
        o->setProperty ("items", toVar (items));
        o->setProperty ("kept", toVar (kept));
        return juce::var (o);
    }

    // The delete button: a fresh scan, only the ticked rows, delete as the user,
    // then once elevated for what is left, then the FL rows whose file is now gone.
    inline juce::var cleanNow (const juce::StringArray& only = {})
    {
        auto items = scan();
        keepOnly (items, only);
        deleteItems (items, false);

        bool needsAdmin = false;
        for (const auto& it : items) needsAdmin = needsAdmin || (it.kind != "daw-list" && it.status == "failed");

        bool elevationDeclined = false;
        if (needsAdmin)
        {
            juce::StringArray rest;
            for (const auto& it : items)
                if (it.kind != "daw-list" && it.status == "failed") rest.add (it.file.getFullPathName());
            auto onlyFile = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                .getChildFile ("RonePlugins").getChildFile ("old-versions-selection.txt");
            onlyFile.getParentDirectory().createDirectory();
            onlyFile.replaceWithText (rest.joinIntoString ("\n"));

            elevationDeclined = ! runElevated (onlyFile);
            onlyFile.deleteFile();
            for (auto& it : items)
                if (it.kind != "daw-list" && it.status == "failed" && ! it.file.exists())
                    it.status = "deleted";
        }

        // FL rows last: only those whose file is gone now; then the row left keeps a plain name
        juce::Array<juce::File> flDirs;
        for (auto& it : items)
        {
            if (it.kind != "daw-list") continue;
            if (it.target != juce::File() && it.target.exists()) { it.status = "kept"; continue; }
            deleteOne (it);
            if (it.status == "deleted") flDirs.addIfNotAlreadyThere (it.file.getParentDirectory());
        }
        flTidyNames (flDirs);

        int deleted = 0, inUse = 0, failed = 0, kept = 0;
        for (const auto& it : items)
        {
            if (it.status == "deleted") ++deleted;
            else if (it.status == "in_use") ++inUse;
            else if (it.status == "kept") ++kept;
            else ++failed;
        }

        auto* o = new juce::DynamicObject();
        o->setProperty ("success", true);
        o->setProperty ("items", toVar (items));
        o->setProperty ("deleted", deleted);
        o->setProperty ("inUse", inUse);
        o->setProperty ("failed", failed);
        o->setProperty ("kept", kept);
        o->setProperty ("elevationDeclined", elevationDeclined);
        return juce::var (o);
    }

    // --delete-old-versions (the elevated instance) and --list-old-versions (a dry run):
    // the result goes to %APPDATA%\RonePlugins\old-versions.json for whoever started it.
    // --only-file "<f>": delete only rows whose path is a line of <f> (unreadable = none).
    // The elevated instance never touches FL rows (another user's, maybe) - the Center
    // that started it does that once it returns.
    inline bool runCommandLine (const juce::String& commandLine)
    {
        const bool doDelete = commandLine.contains (kDeleteFlag);
        const bool doList = commandLine.contains (kListFlag);
        if (! doDelete && ! doList) return false;

        auto tokens = juce::StringArray::fromTokens (commandLine, true);
        const int at = tokens.indexOf ("--test-root");
        const bool test = at >= 0 && at + 1 < tokens.size();
        const juce::File testBase (test ? juce::File (tokens[at + 1].unquoted()) : juce::File());
        const auto roots = test ? testRoots (testBase) : defaultRoots();
        const auto flData = test ? testBase.getChildFile ("Image-Line") : flSharedData();

        auto items = scan (roots, nullptr, flData);

        const int of = tokens.indexOf ("--only-file");
        if (of >= 0)
        {
            const juce::File f (of + 1 < tokens.size() ? tokens[of + 1].unquoted() : juce::String());
            juce::StringArray only;
            if (juce::File::isAbsolutePath (f.getFullPathName()) && f.existsAsFile())
                only.addLines (f.loadFileAsString());
            only.removeEmptyStrings();
            if (only.isEmpty()) items.clear();               // asked for a selection and got none: delete nothing
            else                keepOnly (items, only);
        }

        if (doDelete) deleteItems (items, test);             // the real elevated run leaves FL rows to the caller

        auto out = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                       .getChildFile ("RonePlugins").getChildFile ("old-versions.json");
        out.getParentDirectory().createDirectory();
        out.replaceWithText (juce::JSON::toString (toVar (items)));
        return true;
    }

    inline bool isCommandLineMode (const juce::String& commandLine)
    {
        return commandLine.contains (kDeleteFlag) || commandLine.contains (kListFlag);
    }
}
