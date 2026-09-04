#include "PluginProcessor.h"
#include "PluginEditor.h"

// =============================================================================
// Construction
// =============================================================================
RoneAfterspaceAudioProcessor::RoneAfterspaceAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Standalone builds own their process: capture crashes into the shared
    // report queue (the Plugins Center uploads it). Never installed in a DAW.
    if (wrapperType == wrapperType_Standalone)
        RoneCrashReporter::installCrashHandler (JucePlugin_Name, JucePlugin_VersionString, "Standalone");

    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id); };

    pMix         = raw (ParamIDs::mix);
    pSize        = raw (ParamIDs::size);
    pDecay       = raw (ParamIDs::decay);
    pBloom       = raw (ParamIDs::bloom);
    pSilk        = raw (ParamIDs::silk);
    pAutoGain    = raw (ParamIDs::autoGain);
    pSoloWet     = raw (ParamIDs::soloWet);
    pCharacter   = raw (ParamIDs::character);
    pBypass      = raw (ParamIDs::bypass);
    pFreeze      = raw (ParamIDs::freeze);
    pDuckAmount  = raw (ParamIDs::duckAmount);
    pDuckMode    = raw (ParamIDs::duckMode);
    pDuckAttack  = raw (ParamIDs::duckAttack);
    pDuckHold    = raw (ParamIDs::duckHold);
    pDuckRelease = raw (ParamIDs::duckRelease);
    pDuckSens    = raw (ParamIDs::duckSensitivity);
    pEchoAmount  = raw (ParamIDs::echoAmount);
    pEchoTime    = raw (ParamIDs::echoTime);
    pEchoFreeMs  = raw (ParamIDs::echoFreeMs);
    pEchoFeedback = raw (ParamIDs::echoFeedback);
    pEchoRouting  = raw (ParamIDs::echoRouting);
    pEchoPingPong = raw (ParamIDs::echoPingPong);
    pEchoLowCut   = raw (ParamIDs::echoLowCut);
    pEchoHighCut  = raw (ParamIDs::echoHighCut);
    pPreDelay    = raw (ParamIDs::preDelay);
    pTexture     = raw (ParamIDs::texture);
    pEarlyLate   = raw (ParamIDs::earlyLate);
    pLowCut      = raw (ParamIDs::lowCut);
    pHighCut     = raw (ParamIDs::highCut);
    pLowDamp     = raw (ParamIDs::lowDamp);
    pHighDamp    = raw (ParamIDs::highDamp);
    pModRate     = raw (ParamIDs::modRate);
    pModDepth    = raw (ParamIDs::modDepth);
    pWidth       = raw (ParamIDs::width);
    pMonoLow     = raw (ParamIDs::monoLow);

    // Prime the license/kill-switch cache (constructor = message thread)
    checkBundleLicense();

    // The licence is re-read every minute (message thread) so signing out in the
    // Center, a remote revoke or an expired pass locks a running instance too.
    // Not inside JUCE's VST3 manifest helper: it exits immediately and a live timer thread crashes it.
    if (! RemoteLicenseGate::isRunningInPluginBuildHelper())
        startTimer (60 * 1000);
}

// =============================================================================
// Prepare / Release
// =============================================================================
void RoneAfterspaceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Not the realtime thread yet — refresh the license/kill-switch cache
    checkBundleLicense();

    reverb.prepare (sampleRate, samplesPerBlock);
    echo.prepare (sampleRate, samplesPerBlock);
    duck.prepare (sampleRate);
    silk.prepare (sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    wetLowCut.prepare (spec);
    wetLowCut.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    wetLowCut.setCutoffFrequency (120.0f);
    wetHighCut.prepare (spec);
    wetHighCut.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    wetHighCut.setCutoffFrequency (12000.0f);

    smMix.reset (sampleRate, 0.020);
    smMix.setCurrentAndTargetValue (pMix->load());
    smEchoSend.reset (sampleRate, 0.020);
    smEchoSend.setCurrentAndTargetValue (pEchoAmount->load());
    smWidth.reset (sampleRate, 0.020);
    smWidth.setCurrentAndTargetValue (pWidth->load());
    smBypass.reset (sampleRate, 0.030);
    smBypass.setCurrentAndTargetValue (pBypass->load() > 0.5f ? 1.0f : 0.0f);
    smWetComp.reset (sampleRate, 0.080);
    smWetComp.setCurrentAndTargetValue (1.0f);
    smMonoLow.reset (sampleRate, 0.030);
    smMonoLow.setCurrentAndTargetValue (pMonoLow->load() > 0.5f ? 1.0f : 0.0f);
    smSoloWet.reset (sampleRate, 0.030);
    smSoloWet.setCurrentAndTargetValue (0.0f);

    // Mono-low: one-pole LP at 150Hz on the side signal
    sideLowCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 150.0f / (float) sampleRate);
    sideLowState = 0.0f;

    vizDecimationInterval = juce::jmax (1, (int) (sampleRate * 0.010)); // ~10ms/point
    vizAccum = 0.0f;
    vizAccumCount = 0;
}

void RoneAfterspaceAudioProcessor::releaseResources()
{
    reverb.reset();
    echo.reset();
    duck.reset();
    silk.reset();
}

bool RoneAfterspaceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    auto in = layouts.getMainInputChannelSet();
    return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

// =============================================================================
// Tempo
// =============================================================================
double RoneAfterspaceAudioProcessor::getEffectiveBPM() const
{
    return dawBpmAvailable.load() ? dawBpm.load() : 120.0; // spec §18 fallback
}

double RoneAfterspaceAudioProcessor::getCurrentEchoTimeMs() const
{
    return EchoTimes::timeMs ((int) pEchoTime->load(), getEffectiveBPM(), pEchoFreeMs->load());
}

// =============================================================================
// Process
// =============================================================================
void RoneAfterspaceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    // --- Host tempo ---
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm())
            {
                dawBpm.store (*bpm);
                dawBpmAvailable.store (*bpm > 0.0);
            }
        }
    }

    // Mono-in/stereo-out: duplicate the mono channel so the wet field decorrelates
    if (getTotalNumInputChannels() == 1 && buffer.getNumChannels() >= 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    // Remote kill-switch / license gate: locked -> clean bypass, the dry
    // signal passes through untouched (never silences a customer's mix).
    if (! licenseValid.load (std::memory_order_relaxed))
        return;

    auto* dataL = buffer.getWritePointer (0);
    auto* dataR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : dataL;

    // =========================================================================
    // Block-rate parameter update (macros computed here — spec §6/§9)
    // =========================================================================
    const float character = pCharacter->load();  // 0 clean .. 1 dream
    const float bloom     = pBloom->load();

    // CHARACTER macro: clean = tight/dark/conservative, dream = wide/moving/airy
    const float charModScale   = 0.60f + 0.90f * character;
    const float charWidthScale = 0.90f + 0.30f * character;
    const float charDampScale  = juce::jlimit (0.0f, 1.5f, 1.25f - 0.55f * character);
    const float charFbScale    = 0.80f + 0.45f * character;
    const float bloomScale     = 0.70f + 0.60f * character;

    // TEXTURE drives both density and diffusion (one knob, spec-simplified)
    const float texture = pTexture->load();
    const float baseDiffusion = 0.45f + 0.45f * texture;
    const float baseDensity   = 0.40f + 0.60f * texture;

    // BLOOM macro (MVP mapping per spec §28: modulation + density + width)
    const float bloomEff     = bloom * bloomScale;
    const float modDepthEff  = juce::jlimit (0.0f, 1.5f, pModDepth->load() * charModScale + bloomEff * 0.40f);
    const float densityEff   = juce::jlimit (0.0f, 1.0f, baseDensity + bloomEff * 0.25f);
    const float diffusionEff = juce::jlimit (0.0f, 1.0f, baseDiffusion * (0.95f + 0.15f * character));
    const float widthEff     = juce::jlimit (0.0f, 2.0f, pWidth->load() * charWidthScale * (1.0f + 0.30f * bloomEff));
    const float highDampEff  = juce::jlimit (0.0f, 1.0f, pHighDamp->load() * charDampScale);

    const bool frozen = pFreeze->load() > 0.5f;

    reverb.setSize (pSize->load());
    reverb.setDecaySeconds (pDecay->load());
    reverb.setPreDelayMs (pPreDelay->load());
    reverb.setDiffusion (diffusionEff);
    reverb.setDensity (densityEff);
    reverb.setModRate (pModRate->load());
    reverb.setModDepth (modDepthEff);
    reverb.setHighDamp (highDampEff);
    reverb.setLowDamp (pLowDamp->load());
    reverb.setEarlyLateBalance (pEarlyLate->load());
    reverb.setFreeze (frozen);

    echo.setTimeMs ((float) getCurrentEchoTimeMs());
    echo.setFeedback (juce::jlimit (0.0f, 0.95f, pEchoFeedback->load() * charFbScale));
    echo.setPingPong (pEchoPingPong->load() > 0.5f);
    echo.setLowCutHz (pEchoLowCut->load());
    echo.setHighCutHz (pEchoHighCut->load());

    // Duck mode presets (spec §7 MODES); CUSTOM exposes the advanced params
    duck.setAmount (pDuckAmount->load());
    switch ((int) pDuckMode->load())
    {
        case DuckModes::Smart:
            duck.setAttackMs (8.0f);   duck.setHoldMs (60.0f);  duck.setReleaseMs (380.0f);
            duck.setDetectorFilters (180.0f, 8000.0f);
            break;
        case DuckModes::Full:
            duck.setAttackMs (12.0f);  duck.setHoldMs (40.0f);  duck.setReleaseMs (300.0f);
            duck.setDetectorFilters (20.0f, 20000.0f);
            break;
        case DuckModes::Vocal:
            duck.setAttackMs (15.0f);  duck.setHoldMs (80.0f);  duck.setReleaseMs (450.0f);
            duck.setDetectorFilters (220.0f, 6000.0f);
            break;
        case DuckModes::Transient:
            duck.setAttackMs (0.5f);   duck.setHoldMs (15.0f);  duck.setReleaseMs (140.0f);
            duck.setDetectorFilters (800.0f, 20000.0f);
            break;
        case DuckModes::Custom:
        default:
            duck.setAttackMs (pDuckAttack->load());
            duck.setHoldMs (pDuckHold->load());
            duck.setReleaseMs (pDuckRelease->load());
            duck.setDetectorFilters (20.0f, 20000.0f);
            break;
    }
    duck.setSensitivity (pDuckSens->load());

    wetLowCut.setCutoffFrequency (pLowCut->load());
    wetHighCut.setCutoffFrequency (pHighCut->load());

    silk.setAmount (pSilk->load());

    // AUTO-GAIN: analytic wet compensation so Size/Decay sweeps keep loudness.
    // Tail RMS grows ~sqrt(T60); size adds energy via longer lines. Referenced
    // to the Init preset (decay 2.2s / size 0.55), clamped to +/-9dB. No
    // envelope tracking -> can never pump.
    if (pAutoGain->load() > 0.5f)
    {
        float decayComp = std::sqrt (2.2f / juce::jmax (0.2f, pDecay->load()));
        float sizeComp  = std::sqrt ((0.30f + 0.85f * 0.55f)
                                     / (0.30f + 0.85f * juce::jmax (0.0f, pSize->load())));
        smWetComp.setTargetValue (juce::jlimit (0.355f, 2.82f, decayComp * sizeComp));
    }
    else
    {
        smWetComp.setTargetValue (1.0f);
    }

    smMix.setTargetValue (pMix->load());
    smEchoSend.setTargetValue (pEchoAmount->load());
    smWidth.setTargetValue (widthEff);
    smBypass.setTargetValue (pBypass->load() > 0.5f ? 1.0f : 0.0f);
    smMonoLow.setTargetValue (pMonoLow->load() > 0.5f ? 1.0f : 0.0f);
    smSoloWet.setTargetValue (pSoloWet->load() > 0.5f ? 1.0f : 0.0f);

    const int routing = (int) pEchoRouting->load();

    // =========================================================================
    // Per-sample processing (spec §14 signal flow)
    // =========================================================================
    for (int n = 0; n < numSamples; ++n)
    {
        const float dryL = dataL[n];
        const float dryR = dataR[n];

        // 3. duck detector reads the dry input (spec §14 DUCK DETECTOR)
        const float duckGain = duck.processSample (dryL, dryR);

        const float echoSend = smEchoSend.getNextValue();

        float revInL = dryL, revInR = dryR;
        float echoOutL = 0.0f, echoOutR = 0.0f;
        float echoDirectL = 0.0f, echoDirectR = 0.0f;

        float wetL = 0.0f, wetR = 0.0f;

        if (routing == EchoRouting::After)
        {
            // Reverb -> Delay: the tail itself echoes
            reverb.processSample (revInL, revInR, wetL, wetR);
            echo.processSample (wetL * echoSend, wetR * echoSend, echoOutL, echoOutR);
            wetL += echoOutL;
            wetR += echoOutR;
        }
        else if (routing == EchoRouting::Inside)
        {
            // Repeats dissolve into the space: echo feeds the reverb only
            echo.processSample (dryL * echoSend, dryR * echoSend, echoOutL, echoOutR);
            revInL += echoOutL;
            revInR += echoOutR;
            reverb.processSample (revInL, revInR, wetL, wetR);
        }
        else // Before (default)
        {
            // Dry -> Delay -> Reverb; repeats also stay audible in the wet mix
            echo.processSample (dryL * echoSend, dryR * echoSend, echoOutL, echoOutR);
            revInL += echoOutL;
            revInR += echoOutR;
            echoDirectL = echoOutL * 0.7f;
            echoDirectR = echoOutR * 0.7f;
            reverb.processSample (revInL, revInR, wetL, wetR);
        }

        wetL += echoDirectL;
        wetR += echoDirectR;

        // 10. wet tone shaping
        wetL = wetHighCut.processSample (0, wetLowCut.processSample (0, wetL));
        wetR = wetHighCut.processSample (1, wetLowCut.processSample (1, wetR));

        // 10b. SILK: built-in de-esser on the tail
        silk.processSample (wetL, wetR);

        // 10c. AUTO-GAIN wet compensation
        const float wetComp = smWetComp.getNextValue();
        wetL *= wetComp;
        wetR *= wetComp;

        // 11. ducking gain stage
        wetL *= duckGain;
        wetR *= duckGain;

        // 12. stereo width (M/S) + MONO-LOW (side lows collapse to center)
        const float width = smWidth.getNextValue();
        const float monoLow = smMonoLow.getNextValue();
        const float mid  = 0.5f * (wetL + wetR);
        float side = 0.5f * (wetL - wetR);
        sideLowState += sideLowCoeff * (side - sideLowState);
        side = (side - sideLowState * monoLow) * width;
        wetL = mid + side;
        wetR = mid - side;

        // 13. equal-power wet/dry mix (+ SOLO WET audition crossfade)
        const float mix  = smMix.getNextValue();
        const float solo = smSoloWet.getNextValue();
        const float dryGain = std::cos (mix * juce::MathConstants<float>::halfPi) * (1.0f - solo);
        const float wetGain = std::sin (mix * juce::MathConstants<float>::halfPi) * (1.0f - solo) + solo;

        float outL = dryL * dryGain + wetL * wetGain;
        float outR = dryR * dryGain + wetR * wetGain;

        // Bypass crossfade (click-free)
        const float byp = smBypass.getNextValue();
        outL = outL + byp * (dryL - outL);
        outR = outR + byp * (dryR - outR);

        dataL[n] = outL;
        dataR[n] = outR;

        // Visualizer: wet energy, decimated to ~10ms points
        vizAccum += wetL * wetL + wetR * wetR;
        if (++vizAccumCount >= vizDecimationInterval)
        {
            float rms = std::sqrt (vizAccum / (float) (2 * vizAccumCount));
            int pos = visualizerWritePos.load (std::memory_order_relaxed);
            visualizerRing[(size_t) pos].store (rms, std::memory_order_relaxed);
            visualizerWritePos.store ((pos + 1) % kVisualizerBufferSize, std::memory_order_release);
            vizAccum = 0.0f;
            vizAccumCount = 0;
        }
    }

    currentDuckGrDb.store (duck.getGainReductionDb());
    currentSilkGrDb.store (silk.getGainReductionDb());
    currentWetRms.store (visualizerRing[(size_t) ((visualizerWritePos.load() + kVisualizerBufferSize - 1)
                                                  % kVisualizerBufferSize)].load());

    // 14. output safety: if anything went non-finite, hard-reset the engines
    const float lastL = dataL[numSamples - 1];
    const float lastR = dataR[numSamples - 1];
    if (! std::isfinite (lastL) || ! std::isfinite (lastR))
    {
        reverb.reset();
        echo.reset();
        duck.reset();
        silk.reset();
        sideLowState = 0.0f;
        wetLowCut.reset();
        wetHighCut.reset();
        buffer.clear();
    }
}

// =============================================================================
// Tail / License
// =============================================================================
double RoneAfterspaceAudioProcessor::getTailLengthSeconds() const
{
    return (double) pDecay->load() + getCurrentEchoTimeMs() / 1000.0;
}

void RoneAfterspaceAudioProcessor::checkBundleLicense()
{
    // Remote kill-switch: open mode (versions.json license_mode) plays free;
    // enforced mode requires the real bundle license.
    RemoteLicenseGate::refreshFromNetworkAsync();
    licenseValid.store (RemoteLicenseGate::isOpenMode()
                         || BundleLicenseChecker::isProductLicensed ("RoneAfterspace"));
}

// =============================================================================
// Preset name
// =============================================================================
juce::String RoneAfterspaceAudioProcessor::getPresetName() const
{
    const juce::ScopedLock sl (presetNameLock);
    return presetName;
}

void RoneAfterspaceAudioProcessor::setPresetName (const juce::String& name)
{
    const juce::ScopedLock sl (presetNameLock);
    presetName = name;
}

// =============================================================================
// State (spec §24 — schema versioned; Freeze always restores OFF)
// =============================================================================
void RoneAfterspaceAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto root = std::make_unique<juce::XmlElement> ("RoneAfterspaceState");
    root->setAttribute ("schemaVersion", kStateSchemaVersion);
    root->setAttribute ("presetName", getPresetName());
    root->setAttribute ("uiWidth",  uiWidth.load());
    root->setAttribute ("uiHeight", uiHeight.load());

    auto state = apvts.copyState();
    root->addChildElement (state.createXml().release());

    copyXmlToBinary (*root, destData);
}

void RoneAfterspaceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName ("RoneAfterspaceState"))
        return;

    // schemaVersion 1 is current; future versions migrate here explicitly
    setPresetName (xml->getStringAttribute ("presetName", "Custom"));
    uiWidth.store  (juce::jlimit (805, 1840,  xml->getIntAttribute ("uiWidth", 1150)));
    uiHeight.store (juce::jlimit (546, 1248,  xml->getIntAttribute ("uiHeight", 780)));

    if (auto* params = xml->getChildByName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*params));

    // Spec §34: restoring Freeze ON is confusing — captured buffers aren't saved
    if (auto* freezeParam = apvts.getParameter (ParamIDs::freeze))
        freezeParam->setValueNotifyingHost (0.0f);

    // Solo Wet is an audition tool — never restore it engaged
    if (auto* soloParam = apvts.getParameter (ParamIDs::soloWet))
        soloParam->setValueNotifyingHost (0.0f);
}

// =============================================================================
juce::AudioProcessorEditor* RoneAfterspaceAudioProcessor::createEditor()
{
    return new RoneAfterspaceAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RoneAfterspaceAudioProcessor();
}
