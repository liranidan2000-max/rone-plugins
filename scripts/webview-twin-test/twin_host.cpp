// ============================================================================
// TwinEditorHost - the JUCE 8.0.4 WebView2 serial-creation trap, as a test.
//
// Loads ONE VST3 and makes N instances of it (same DLL, so the same JUCE statics),
// then creates all N editors and their windows in the SAME message-loop turn -
// what a DAW does when it restores a project with several windows of one plugin
// open - and hosts them for a while (audio blocks + message loop).
//
//   TwinEditorHost <plugin.vst3> <instances> <seconds>
//
// The check is made from outside (cdp_check.mjs): with
// WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS=--remote-debugging-port=<port> every editor
// must own one https://juce.backend/ page. A window whose WebView2 was never built
// has no page at all - it stays blank forever.
// Run it with WEBVIEW2_USER_DATA_FOLDER=<scratch dir> (its own browser process, never
// the profile of a plugin open in a DAW) and on a hidden desktop (HiddenDesktopRun).
// ============================================================================
#include <JuceHeader.h>

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    if (argc < 4)
    {
        std::cout << "usage: TwinEditorHost <plugin.vst3> <instances> <seconds>" << std::endl;
        return 2;
    }

    const juce::String path (argv[1]);
    const int instances  = juce::jlimit (1, 8, juce::String (argv[2]).getIntValue());
    const double seconds = juce::String (argv[3]).getDoubleValue();

    juce::AudioPluginFormatManager formats;
    formats.addDefaultFormats();
    juce::VST3PluginFormat vst3;
    juce::OwnedArray<juce::PluginDescription> descs;
    vst3.findAllTypesForFile (descs, path);
    if (descs.isEmpty()) { std::cout << "no plugin found in " << path << std::endl; return 2; }
    std::cout << "plugin   " << descs[0]->name << " " << descs[0]->version << "  (" << path << ")" << std::endl;

    std::vector<std::unique_ptr<juce::AudioPluginInstance>> plugs;
    for (int i = 0; i < instances; ++i)
    {
        juce::String err;
        auto p = formats.createPluginInstance (*descs[0], 48000.0, 256, err);
        if (p == nullptr) { std::cout << "load failed: " << err << std::endl; return 2; }
        p->prepareToPlay (48000.0, 256);
        plugs.push_back (std::move (p));
    }

    // Every editor and its window in ONE turn: nothing is dispatched in between.
    std::vector<std::unique_ptr<juce::AudioProcessorEditor>> editors;
    std::vector<std::unique_ptr<juce::DocumentWindow>> windows;
    for (int i = 0; i < instances; ++i)
    {
        editors.emplace_back (plugs[(size_t) i]->createEditorIfNeeded());
        if (editors.back() == nullptr) { std::cout << "instance " << i + 1 << " has no editor" << std::endl; return 2; }

        auto w = std::make_unique<juce::DocumentWindow> ("TwinEditorHost " + juce::String (i + 1),
                                                         juce::Colours::black, juce::DocumentWindow::closeButton);
        w->setUsingNativeTitleBar (true);
        w->setContentNonOwned (editors.back().get(), true);
        w->setTopLeftPosition (40 + 70 * i, 40 + 70 * i);
        w->setVisible (true);
        windows.push_back (std::move (w));
    }
    std::cout << "editors  " << instances << " created in one message-loop turn, hosting for "
              << juce::String (seconds, 1) << " s" << std::endl;

    juce::AudioBuffer<float> buffer (2, 256);
    const double start = juce::Time::getMillisecondCounterHiRes() * 0.001;
    while (juce::Time::getMillisecondCounterHiRes() * 0.001 - start < seconds)
    {
        for (auto& p : plugs)
        {
            juce::MidiBuffer midi;
            buffer.clear();
            p->processBlock (buffer, midi);
        }
        juce::MessageManager::getInstance()->runDispatchLoopUntil (16);
    }

    windows.clear();
    editors.clear();                 // editors go before their processors
    plugs.clear();
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
    std::cout << "closed   all editors and instances" << std::endl;
    return 0;
}
