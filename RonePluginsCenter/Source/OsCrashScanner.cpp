#include "OsCrashScanner.h"
#include "../../Shared/RoneCrashReporter.h"
#include <atomic>

#if JUCE_WINDOWS
 #include <windows.h>
 #include <winevt.h>
 #pragma comment (lib, "wevtapi.lib")
#endif

namespace OsCrashScanner
{

namespace
{
constexpr int maxPerPass = 10;      // a machine that crashed all week must not flood the queue
constexpr int repeatDays = 7;       // the same crash from the same machine, once a week

bool isRoneName (const juce::String& s)
{
    return s.startsWith ("RONE ") || s.startsWith ("RONE_") || s.startsWith ("ReverseReverb")
        || s.startsWithIgnoreCase ("kalifonic")
        || (s.startsWith ("Rone") && juce::CharacterFunctions::isUpperCase (s[4]));
}

juce::String stem (const juce::String& fileName)
{
    return fileName.upToLastOccurrenceOf (".", false, false);
}

#if JUCE_WINDOWS
// The VST Manager hosts other people's plugins on purpose, so what dies inside it is
// usually theirs: it counts only when the faulting module is ours, like any other host.
bool isOwnApp (const juce::String& app)
{
    return isRoneName (app) && ! app.containsIgnoreCase ("VST MANAGER");
}

bool isSystemModule (const juce::String& module)
{
    return module.equalsIgnoreCase ("ntdll.dll") || module.equalsIgnoreCase ("KERNELBASE.dll")
        || module.equalsIgnoreCase ("ucrtbase.dll") || module.startsWithIgnoreCase ("VCRUNTIME140")
        || module.startsWithIgnoreCase ("MSVCP140") || module.equalsIgnoreCase ("unknown");
}

juce::String renderXml (EVT_HANDLE event)
{
    DWORD used = 0, properties = 0;
    EvtRender (nullptr, event, EvtRenderEventXml, 0, nullptr, &used, &properties);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return {};

    juce::HeapBlock<wchar_t> buffer (used / sizeof (wchar_t) + 1, true);
    if (! EvtRender (nullptr, event, EvtRenderEventXml, used, buffer.get(), &used, &properties))
        return {};
    return juce::String (buffer.get());
}

void readEvent (const juce::String& xml, juce::int64& recordId, juce::StringPairArray& data)
{
    auto root = juce::parseXML (xml);
    if (root == nullptr)
        return;

    if (auto* system = root->getChildByName ("System"))
        if (auto* id = system->getChildByName ("EventRecordID"))
            recordId = id->getAllSubText().getLargeIntValue();

    if (auto* eventData = root->getChildByName ("EventData"))
        for (auto* item : eventData->getChildIterator())
            data.set (item->getStringAttribute ("Name"), item->getAllSubText());
}

void scanWindows (const juce::File& stateFile, const std::function<void (const Crash&)>& sink)
{
    const auto lastSeen = stateFile.loadFileAsString().trim().getLargeIntValue();

    const wchar_t* query = L"*[System[Provider[@Name='Application Error'] and (EventID=1000)"
                           L" and TimeCreated[timediff(@SystemTime) <= 1209600000]]]";   // 14 days

    const auto results = EvtQuery (nullptr, L"Application", query, EvtQueryChannelPath | EvtQueryReverseDirection);
    if (results == nullptr)
        return;

    juce::int64 newest = lastSeen;
    int reported = 0, read = 0;
    bool done = false;

    while (! done && read < 300)
    {
        EVT_HANDLE events[16];
        DWORD returned = 0;
        if (! EvtNext (results, 16, events, 2000, 0, &returned))
            break;

        for (DWORD i = 0; i < returned; ++i)
        {
            juce::int64 recordId = 0;
            juce::StringPairArray data;
            readEvent (renderXml (events[i]), recordId, data);
            EvtClose (events[i]);
            ++read;

            newest = juce::jmax (newest, recordId);
            if (recordId <= lastSeen)
            {
                done = true;
                continue;
            }

            const auto app = data["AppName"];
            const auto module = data["ModuleName"];
            const auto moduleIsOurs = isRoneName (module);

            if (! moduleIsOurs && ! (isOwnApp (app) && isSystemModule (module)))
                continue;

            if (reported >= maxPerPass)
                continue;
            ++reported;

            const auto where = module + "+0x" + data["FaultingOffset"].trimCharactersAtStart ("0");

            Crash c;
            c.product = stem (moduleIsOurs ? module : app);
            c.version = moduleIsOurs ? data["ModuleVersion"] : data["AppVersion"];
            c.wrapper = module.endsWithIgnoreCase (".vst3") ? "VST3" : "Standalone";
            c.message = where + " exception 0x" + data["ExceptionCode"] + " in " + app;
            c.details = "host: " + app + " " + data["AppVersion"]
                      + "\nexception: 0x" + data["ExceptionCode"]
                      + "\ncrashed at: " + where
                      + "\nbuild: " + data["ModuleTimeStamp"].toUpperCase()
                      + "\nsource: Windows Application Error event";
            c.stack = where;
            sink (c);
        }
    }

    EvtClose (results);

    if (newest != lastSeen)
        stateFile.replaceWithText (juce::String (newest));
}
#endif

#if JUCE_MAC
juce::String imageName (const juce::var& image)
{
    return image["name"].toString();
}

void readIps (const juce::File& file, const std::function<void (const Crash&)>& sink, int& reported)
{
    const auto text = file.loadFileAsString();
    const auto split = text.indexOfChar ('\n');      // a header line, then the report
    if (split <= 0)
        return;

    const auto header = juce::JSON::parse (text.substring (0, split));
    if (header["bug_type"].toString() != "309")      // 309 is a crash
        return;

    const auto body = juce::JSON::parse (text.substring (split + 1));
    const auto* images = body["usedImages"].getArray();
    const auto* threads = body["threads"].getArray();
    const int faulting = body["faultingThread"];
    if (images == nullptr || threads == nullptr || ! juce::isPositiveAndBelow (faulting, threads->size()))
        return;

    const auto* frames = (*threads)[faulting]["frames"].getArray();
    if (frames == nullptr)
        return;

    juce::StringArray stack;
    juce::String first, uuid, product, version;

    for (int i = 0; i < juce::jmin (32, frames->size()); ++i)
    {
        const auto& frame = (*frames)[i];
        const int index = frame["imageIndex"];
        if (! juce::isPositiveAndBelow (index, images->size()))
            continue;

        const auto& image = (*images)[index];
        const auto name = imageName (image);
        const auto where = name + "+0x" + juce::String::toHexString ((juce::int64) frame["imageOffset"]);
        stack.add (where);

        if (first.isEmpty() && isRoneName (name))
        {
            first = where;
            uuid = image["uuid"].toString();
            product = name;
            version = image["CFBundleShortVersionString"].toString();
        }
    }

    const auto process = header["app_name"].toString();
    if (first.isEmpty() || ++reported > maxPerPass)
        return;

    const auto exception = body["exception"]["type"].toString() + " " + body["exception"]["signal"].toString();

    Crash c;
    c.product = product;
    c.version = version.isNotEmpty() ? version : "unknown";
    c.wrapper = process == product ? "Standalone" : "AU/VST3";
    c.message = first + " " + exception + " in " + process;
    c.details = "host: " + process + " " + header["app_version"].toString()
              + "\nexception: " + exception
              + "\ncrashed at: " + stack[0]
              + "\nbuild: uuid " + uuid
              + "\nsource: macOS crash report";
    c.stack = stack.joinIntoString ("\n");
    sink (c);
}

void scanMac (const juce::File& stateFile, const std::function<void (const Crash&)>& sink)
{
    const auto lastScan = juce::Time (stateFile.loadFileAsString().trim().getLargeIntValue());
    const auto started = juce::Time::getCurrentTime();
    const auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Logs/DiagnosticReports");

    auto oldest = juce::jmax (lastScan, started - juce::RelativeTime::days (14));
    int reported = 0;

    for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.ips"))
        if (f.getLastModificationTime() > oldest && f.getSize() < 4 * 1024 * 1024)
            readIps (f, sink, reported);

    stateFile.replaceWithText (juce::String (started.toMilliseconds()));
}
#endif
}

void scan (const juce::File& stateFile, const std::function<void (const Crash&)>& sink)
{
   #if JUCE_WINDOWS
    scanWindows (stateFile, sink);
   #elif JUCE_MAC
    scanMac (stateFile, sink);
   #else
    juce::ignoreUnused (stateFile, sink);
   #endif
}

void scanAsync()
{
    static std::atomic<bool> running { false };
    if (running.exchange (true))
        return;

    juce::Thread::launch ([]
    {
        scan (RoneCrashReporter::getReportsRoot().getChildFile ("os-scan.txt"), [] (const Crash& c)
        {
            const auto key = juce::String::toHexString (c.message.hashCode64());
            auto stamp = RoneCrashReporter::getReportsRoot().getChildFile ("throttle").getChildFile ("os" + key + ".t");

            if (stamp.existsAsFile()
                && juce::Time::getCurrentTime() - stamp.getLastModificationTime() < juce::RelativeTime::days (repeatDays))
                return;

            stamp.getParentDirectory().createDirectory();
            stamp.replaceWithText ("1");
            RoneCrashReporter::writeReport ("crash", c.product, c.version, c.wrapper, "OS_CRASH", c.message, c.details, c.stack);
        });
        running = false;
    });
}

} // namespace OsCrashScanner
