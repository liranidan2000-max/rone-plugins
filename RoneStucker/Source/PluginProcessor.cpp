#include "PluginProcessor.h"
#include "PluginEditor.h"

// ============================================================================
// Parameters
// ============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout RoneStuckerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // The one knob
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        RoneParamIDs::AMOUNT, "Amount",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        RoneParamIDs::BYPASS, "Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        RoneParamIDs::SYNC, "Sync", true));

    // Advanced
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        RoneParamIDs::SMOOTH, "Smooth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        RoneParamIDs::LENGTH, "Length",
        juce::StringArray { "1/1", "1/2", "1/4", "1/8" }, 2));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        RoneParamIDs::RANGE, "Range",
        juce::NormalisableRange<float> (3.0f, 10.0f, 0.1f), 8.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        RoneParamIDs::LOWCUT, "Low Cut",
        juce::NormalisableRange<float> (20.0f, 500.0f, 1.0f, 0.5f), 20.0f));

    return { params.begin(), params.end() };
}

RoneStuckerAudioProcessor::RoneStuckerAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    amountParam = apvts.getRawParameterValue (RoneParamIDs::AMOUNT);
    bypassParam = apvts.getRawParameterValue (RoneParamIDs::BYPASS);
    syncParam   = apvts.getRawParameterValue (RoneParamIDs::SYNC);
    smoothParam = apvts.getRawParameterValue (RoneParamIDs::SMOOTH);
    lengthParam = apvts.getRawParameterValue (RoneParamIDs::LENGTH);
    rangeParam  = apvts.getRawParameterValue (RoneParamIDs::RANGE);
    lowcutParam = apvts.getRawParameterValue (RoneParamIDs::LOWCUT);
}

// ============================================================================
// Audio lifecycle
// ============================================================================
void RoneStuckerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void RoneStuckerAudioProcessor::releaseResources()
{
    engine.reset();
}

bool RoneStuckerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void RoneStuckerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // Tempo + musical position from the host (fallback 120, free capture)
    double bpm = 120.0;
    double ppq = -1.0;
    bool   hostPlaying = false;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto hostBpm = pos->getBpm())
                if (*hostBpm > 0.0)
                    bpm = *hostBpm;

            hostPlaying = pos->getIsPlaying();
            if (auto hostPpq = pos->getPpqPosition())
                ppq = *hostPpq;
        }
    }

    // Capture division → beats: 1/1 = 4 beats, 1/2 = 2, 1/4 = 1, 1/8 = 0.5
    static constexpr float divisionBeats[] = { 4.0f, 2.0f, 1.0f, 0.5f };
    const int divIndex = juce::jlimit (0, 3, (int) lengthParam->load());
    const double beatsPerDiv = divisionBeats[divIndex];
    const double samplesPerBeat = (60.0 / bpm) * currentSampleRate;
    const float baseLenSamples = (float) (beatsPerDiv * samplesPerBeat);

    // SYNC: samples elapsed since the last grid line of the division.
    // Negative = free capture (sync off, transport stopped, or no ppq).
    float syncOffsetSamples = -1.0f;
    if (syncParam->load() > 0.5f && hostPlaying && ppq >= 0.0)
    {
        const double posInDiv = std::fmod (ppq, beatsPerDiv);
        syncOffsetSamples = (float) (posInDiv * samplesPerBeat);
    }

    // Bypass keeps the engine running with amount 0 — the capture buffer stays
    // fresh and re-enabling is click-free (wet gain just ramps back in).
    const bool bypassed = bypassParam->load() > 0.5f;

    engine.setParameters (bypassed ? 0.0f : amountParam->load(),
                          smoothParam->load(),
                          rangeParam->load(),
                          lowcutParam->load(),
                          baseLenSamples,
                          syncOffsetSamples);

    engine.process (buffer);
}

// ============================================================================
// State
// ============================================================================
void RoneStuckerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void RoneStuckerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* RoneStuckerAudioProcessor::createEditor()
{
    return new RoneStuckerAudioProcessorEditor (*this);
}

// ============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RoneStuckerAudioProcessor();
}
