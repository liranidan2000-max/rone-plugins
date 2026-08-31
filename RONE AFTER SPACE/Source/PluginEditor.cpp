#include "PluginEditor.h"
#include "WebUI.h"

// =============================================================================
// Editor Construction
// =============================================================================
RoneAfterspaceAudioProcessorEditor::RoneAfterspaceAudioProcessorEditor (RoneAfterspaceAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      webView (juce::WebBrowserComponent::Options{}
          .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
          .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}
              .withUserDataFolder (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                  .getChildFile ("RonePlugins")
                  .getChildFile ("RoneAfterspace_WebView2")))
          .withNativeIntegrationEnabled()
          .withResourceProvider ([this] (const juce::String& url)
          {
              return getResource (url);
          })
          // JS -> C++: generic APVTS parameter set (real-world values)
          .withEventListener ("setParameter", [this] (const juce::var& payload)
          {
              auto name  = payload["name"].toString();
              auto value = static_cast<float> (payload["value"]);

              if (auto* param = processorRef.apvts.getParameter (name))
                  param->setValueNotifyingHost (param->convertTo0to1 (value));
          })
          // JS -> C++: automation gestures (DAW touch-automation friendly)
          .withEventListener ("beginGesture", [this] (const juce::var& payload)
          {
              if (auto* param = processorRef.apvts.getParameter (payload["name"].toString()))
                  param->beginChangeGesture();
          })
          .withEventListener ("endGesture", [this] (const juce::var& payload)
          {
              if (auto* param = processorRef.apvts.getParameter (payload["name"].toString()))
                  param->endChangeGesture();
          })
          // JS -> C++: preset name (serialized with session state)
          .withEventListener ("setPresetName", [this] (const juce::var& payload)
          {
              processorRef.setPresetName (payload["name"].toString());
          })
          // JS -> C++: corner-grip resize (the WebView covers JUCE's resizer)
          .withEventListener ("requestResize", [this] (const juce::var& payload)
          {
              int w = juce::jlimit (kMinWidth,  kMaxWidth,  (int) payload["width"]);
              int h = juce::jlimit (kMinHeight, kMaxHeight, (int) payload["height"]);
              // Keep the plugin's aspect ratio
              h = juce::roundToInt ((double) w * kHeight / kWidth);
              h = juce::jlimit (kMinHeight, kMaxHeight, h);
              setSize (w, h + (customTitleBar != nullptr ? CustomTitleBar::kHeight : 0));
          })
          .withEventListener ("showAbout", [this] (const juce::var&)
          {
              aboutOverlay.setBounds (getLocalBounds());
              aboutOverlay.show();
          })
          .withEventListener ("launchCenter", [] (const juce::var&)
          {
              RoneAboutOverlay::launchPluginsCenter();
          })
          .withEventListener ("openExternalUrl", [] (const juce::var& payload)
          {
              auto url = payload["url"].toString();
              if (url.isNotEmpty())
                  juce::URL (url).launchInDefaultBrowser();
          }))
{
    isStandalone = (processorRef.wrapperType == juce::AudioProcessor::wrapperType_Standalone);

    addAndMakeVisible (webView);

    addChildComponent (aboutOverlay);
    aboutOverlay.onShow    = [this] { webView.setVisible (false); };
    aboutOverlay.onDismiss = [this] { webView.setVisible (true); };

    // Restore last UI size (spec §24), aspect re-locked to the base ratio
    int startW = juce::jlimit (kMinWidth, kMaxWidth, processorRef.uiWidth.load());
    int startH = juce::jlimit (kMinHeight, kMaxHeight,
                               juce::roundToInt ((double) startW * kHeight / kWidth));

    if (isStandalone)
    {
        customTitleBar = std::make_unique<CustomTitleBar>();
        addAndMakeVisible (*customTitleBar);
        customTitleBar->toFront (false);
        setSize (startW, startH + CustomTitleBar::kHeight);
    }
    else
    {
        // Host-side resizing (window frame / host grip); aspect locked
        setResizable (true, false);
        if (auto* c = getConstrainer())
            c->setFixedAspectRatio ((double) kWidth / (double) kHeight);
        setResizeLimits (kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);
        setSize (startW, startH);
    }

    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    startTimerHz (30);
    processorRef.checkBundleLicense();
}

RoneAfterspaceAudioProcessorEditor::~RoneAfterspaceAudioProcessorEditor()
{
    stopTimer();
}

// =============================================================================
// Resource Provider
// =============================================================================
std::optional<juce::WebBrowserComponent::Resource>
RoneAfterspaceAudioProcessorEditor::getResource (const juce::String& url)
{
    if (url == "/" || url.isEmpty() || url == "/index.html")
    {
        juce::String html (WebUI::getIndexHTML());
        auto utf8 = html.toUTF8();
        auto* rawBytes = reinterpret_cast<const std::byte*> (utf8.getAddress());
        auto numBytes  = static_cast<size_t> (utf8.sizeInBytes());

        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (rawBytes, rawBytes + numBytes),
            juce::String ("text/html")
        };
    }

    return std::nullopt;
}

// =============================================================================
// Layout
// =============================================================================
void RoneAfterspaceAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff14161A)); // RONE Graphite base
}

void RoneAfterspaceAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    if (customTitleBar != nullptr)
        customTitleBar->setBounds (area.removeFromTop (CustomTitleBar::kHeight));

    webView.setBounds (area);
    aboutOverlay.setBounds (getLocalBounds());

    // Remember the content size for session recall (spec §24)
    if (area.getWidth() >= kMinWidth && area.getHeight() >= kMinHeight)
    {
        processorRef.uiWidth.store (area.getWidth());
        processorRef.uiHeight.store (area.getHeight());
    }
}

void RoneAfterspaceAudioProcessorEditor::parentHierarchyChanged()
{
    juce::AudioProcessorEditor::parentHierarchyChanged();

    if (isStandalone && customTitleBar != nullptr
        && customTitleBar->getWindowToDrag() == nullptr)
    {
        if (auto* topLevel = getTopLevelComponent())
        {
            if (auto* dw = dynamic_cast<juce::DocumentWindow*> (topLevel))
            {
                dw->setUsingNativeTitleBar (false);
                dw->setTitleBarHeight (0);
                dw->setResizable (true, false);

                if (auto* sizeConstrainer = dw->getConstrainer())
                {
                    sizeConstrainer->setMinimumSize (kMinWidth, kMinHeight + CustomTitleBar::kHeight);
                    sizeConstrainer->setMaximumSize (kMaxWidth, kMaxHeight + CustomTitleBar::kHeight);
                }

                customTitleBar->setWindowToDrag (dw);
                dw->setContentComponentSize (getWidth(), getHeight());
            }
        }
    }
}

// =============================================================================
// Timer — push state to JS at 30Hz
// =============================================================================
void RoneAfterspaceAudioProcessorEditor::timerCallback()
{
    sendAllParametersToJS();

    // BPM / echo time state
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("effectiveBPM", processorRef.getEffectiveBPM());
        obj->setProperty ("dawAvailable", processorRef.dawBpmAvailable.load());
        obj->setProperty ("echoTimeMs",   processorRef.getCurrentEchoTimeMs());
        obj->setProperty ("presetName",   processorRef.getPresetName());
        webView.emitEventIfBrowserIsVisible ("hostState", obj);
    }

    // Visualizer data
    {
        juce::Array<juce::var> energyArr;
        int readPos = processorRef.visualizerWritePos.load (std::memory_order_acquire);
        for (int i = 0; i < kVisualizerPoints; ++i)
        {
            int idx = (readPos - kVisualizerPoints + i + kVisualizerBufferSize) % kVisualizerBufferSize;
            energyArr.add (processorRef.visualizerRing[(size_t) idx].load (std::memory_order_relaxed));
        }

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("energyProfile", energyArr);
        obj->setProperty ("wetRms",        (double) processorRef.currentWetRms.load());
        obj->setProperty ("duckGrDb",      (double) processorRef.currentDuckGrDb.load());
        webView.emitEventIfBrowserIsVisible ("visualizerData", obj);
    }

    if (++licenseCheckCounter >= kLicenseCheckInterval)
    {
        licenseCheckCounter = 0;
        processorRef.checkBundleLicense();
        sendLicenseStatusToJS();
    }
}

void RoneAfterspaceAudioProcessorEditor::sendAllParametersToJS()
{
    auto* obj = new juce::DynamicObject();

    for (auto* p : processorRef.getParameters())
    {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            obj->setProperty (ranged->paramID,
                              ranged->convertFrom0to1 (ranged->getValue()));
    }

    webView.emitEventIfBrowserIsVisible ("parameterState", obj);
}

void RoneAfterspaceAudioProcessorEditor::sendLicenseStatusToJS()
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("licensed", processorRef.licenseValid.load());
    webView.emitEventIfBrowserIsVisible ("licenseStatus", obj);
}
