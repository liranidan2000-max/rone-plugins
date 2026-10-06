// ============================================================================
// rone-preview-render - plays a WAV through an installed RONE VST3, offline,
// the way a DAW would (a running transport at a set tempo, bar 1 at sample 0),
// and writes the result. Made for the Plugins Center's dry/wet previews
// (Center 2.0): the same loop, once untouched and once through the plugin.
//
//   rone-preview-render --plugin "<.vst3>" --in dry.wav --out wet.wav --bpm 142
//                       [--set "NAME=0.5;OTHER=1"] [--auto "NAME@beat=value;..."]
//                       [--tail 4] [--list]
//
// --set   normalised 0..1 values by parameter name (case-insensitive; the name
//         as the plugin reports it). --auto  the same, at a beat (quarter notes
//         from bar 1), held until the next point - a knob drawn in automation.
// --tail  seconds of silence fed after the loop, so a tail can ring out.
// --list  prints every parameter with its current value and exits.
// ============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>

struct Transport : juce::AudioPlayHead
{
    double bpm = 120.0, sampleRate = 44100.0;
    juce::int64 samplePos = 0;

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm);
        p.setTimeSignature (TimeSignature { 4, 4 });
        p.setIsPlaying (true);
        p.setTimeInSamples (samplePos);
        p.setTimeInSeconds ((double) samplePos / sampleRate);
        const double ppq = (double) samplePos / sampleRate * bpm / 60.0;
        p.setPpqPosition (ppq);
        p.setPpqPositionOfLastBarStart (std::floor (ppq / 4.0) * 4.0);
        p.setBarCount ((juce::int64) std::floor (ppq / 4.0));
        return p;
    }
};

struct Point { juce::String name; double beat; float value; };

static juce::String arg (const juce::StringArray& a, const char* key, const juce::String& fallback = {})
{
    const int i = a.indexOf (key);
    return i >= 0 && i + 1 < a.size() ? a[i + 1].unquoted() : fallback;
}

static juce::AudioProcessorParameter* findParam (juce::AudioPluginInstance& p, const juce::String& name)
{
    for (auto* prm : p.getParameters())
        if (prm->getName (128).trim().equalsIgnoreCase (name.trim()))
            return prm;
    return nullptr;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;   // VST3 editors are never opened, but the plugins expect a message manager
    juce::StringArray a;
    for (int i = 1; i < argc; ++i) a.add (juce::String::fromUTF8 (argv[i]));

    juce::AudioPluginFormatManager formats;
    formats.addFormat (new juce::VST3PluginFormat());
    juce::OwnedArray<juce::PluginDescription> found;
    juce::VST3PluginFormat vst3;
    vst3.findAllTypesForFile (found, arg (a, "--plugin"));
    if (found.isEmpty()) { std::fprintf (stderr, "no plugin in %s\n", arg (a, "--plugin").toRawUTF8()); return 2; }

    juce::AudioFormatManager audioFormats;
    audioFormats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (audioFormats.createReaderFor (juce::File (arg (a, "--in"))));
    if (reader == nullptr && ! a.contains ("--list")) { std::fprintf (stderr, "cannot read --in\n"); return 2; }

    const double sr = reader ? reader->sampleRate : 44100.0;
    const int block = 256;

    juce::String error;
    auto plugin = formats.createPluginInstance (*found[0], sr, block, error);
    if (plugin == nullptr) { std::fprintf (stderr, "load failed: %s\n", error.toRawUTF8()); return 2; }

    if (a.contains ("--list"))
    {
        for (auto* prm : plugin->getParameters())
            std::printf ("%-28s %.3f  %s\n", prm->getName (128).toRawUTF8(), prm->getValue(), prm->getCurrentValueAsText().toRawUTF8());
        return 0;
    }

    plugin->setPlayConfigDetails (2, 2, sr, block);
    plugin->enableAllBuses();
    Transport transport;
    transport.bpm = arg (a, "--bpm", "120").getDoubleValue();
    transport.sampleRate = sr;
    plugin->setPlayHead (&transport);
    plugin->prepareToPlay (sr, block);

    for (auto kv : juce::StringArray::fromTokens (arg (a, "--set"), ";", "\""))
    {
        if (kv.trim().isEmpty()) continue;
        auto name = kv.upToFirstOccurrenceOf ("=", false, false);
        auto* prm = findParam (*plugin, name);
        if (prm == nullptr) { std::fprintf (stderr, "no parameter '%s'\n", name.toRawUTF8()); return 3; }
        prm->setValueNotifyingHost (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue());
    }

    std::vector<Point> autos;
    for (auto kv : juce::StringArray::fromTokens (arg (a, "--auto"), ";", "\""))
    {
        if (kv.trim().isEmpty()) continue;
        const auto name = kv.upToFirstOccurrenceOf ("@", false, false);
        const auto beat = kv.fromFirstOccurrenceOf ("@", false, false).upToFirstOccurrenceOf ("=", false, false).getDoubleValue();
        const auto val  = kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue();
        if (findParam (*plugin, name) == nullptr) { std::fprintf (stderr, "no parameter '%s'\n", name.toRawUTF8()); return 3; }
        autos.push_back ({ name, beat, val });
    }

    const juce::int64 inLen = (juce::int64) reader->lengthInSamples;
    const juce::int64 total = inLen + (juce::int64) (arg (a, "--tail", "0").getDoubleValue() * sr);
    juce::AudioBuffer<float> in (2, (int) total), out (2, (int) total);
    in.clear();
    reader->read (&in, 0, (int) inLen, 0, true, true);

    // As many channels as the plugin has on any bus (a sidechain input counts),
    // or it may refuse the block - Throw's DUCK KEY input did.
    std::printf ("buses: %d in / %d out channels\n", plugin->getTotalNumInputChannels(), plugin->getTotalNumOutputChannels());
    juce::AudioBuffer<float> buf (juce::jmax (2, plugin->getTotalNumInputChannels(), plugin->getTotalNumOutputChannels()), block);
    juce::MidiBuffer midi;
    for (juce::int64 pos = 0; pos < total; pos += block)
    {
        const int n = (int) juce::jmin ((juce::int64) block, total - pos);
        transport.samplePos = pos;
        const double beat = (double) pos / sr * transport.bpm / 60.0;
        for (auto& pt : autos)
            if (beat >= pt.beat && beat < pt.beat + (double) block / sr * transport.bpm / 60.0 + 1e-9)
                findParam (*plugin, pt.name)->setValueNotifyingHost (pt.value);

        buf.setSize (buf.getNumChannels(), n, false, false, true);
        buf.clear();
        for (int c = 0; c < 2; ++c) buf.copyFrom (c, 0, in, c, (int) pos, n);
        midi.clear();
        plugin->processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, (int) pos, buf, c, 0, n);
    }
    plugin->releaseResources();

    juce::File outFile (arg (a, "--out"));
    outFile.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (outFile), sr, 2, 24, {}, 0));
    if (writer == nullptr) { std::fprintf (stderr, "cannot write --out\n"); return 2; }
    writer->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
    std::printf ("ok %s (%.1f s)\n", outFile.getFullPathName().toRawUTF8(), (double) total / sr);
    return 0;
}
