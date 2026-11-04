#include <juce_core/juce_core.h>
#include "../../../RonePluginsCenter/Source/OsCrashScanner.h"

int main()
{
    auto state = juce::File::createTempFile ("os-scan-state");
    int count = 0;

    OsCrashScanner::scan (state, [&] (const OsCrashScanner::Crash& c)
    {
        ++count;
        std::printf ("[%s %s %s] %s\n%s\n\n", c.product.toRawUTF8(), c.version.toRawUTF8(), c.wrapper.toRawUTF8(), c.message.toRawUTF8(), c.details.toRawUTF8());
    });
    std::printf ("%d crashes, state %s\n", count, state.loadFileAsString().toRawUTF8());

    int again = 0;
    OsCrashScanner::scan (state, [&] (const OsCrashScanner::Crash&) { ++again; });
    std::printf ("second pass: %d\n", again);
    state.deleteFile();
    return 0;
}
