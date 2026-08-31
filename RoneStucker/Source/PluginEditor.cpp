#include "PluginEditor.h"
#include "WebUI.h"

RoneStuckerAudioProcessorEditor::RoneStuckerAudioProcessorEditor (RoneStuckerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      webView (juce::WebBrowserComponent::Options{}
          .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
          .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}
              .withUserDataFolder (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                  .getChildFile ("RonePlugins")
                  .getChildFile ("RoneStucker_WebView2")))
          .withNativeIntegrationEnabled()
          .withResourceProvider ([this] (const juce::String& url)
          {
              return getResource (url);
          })
          // JS -> C++: parameter changes
          .withEventListener ("setParameter", [this] (const juce::var& payload)
          {
              auto name  = payload["name"].toString();
              auto value = static_cast<float> (payload["value"]);

              if (auto* param = processorRef.apvts.getParameter (name))
                  param->setValueNotifyingHost (param->convertTo0to1 (value));
          })
          // JS -> C++: begin/end gesture (for host automation recording)
          .withEventListener ("paramGesture", [this] (const juce::var& payload)
          {
              auto name  = payload["name"].toString();
              bool begin = payload["begin"];

              if (auto* param = processorRef.apvts.getParameter (name))
              {
                  if (begin) param->beginChangeGesture();
                  else       param->endChangeGesture();
              }
          })
          // JS -> C++: UI is ready, push current state
          .withEventListener ("requestState", [this] (const juce::var&)
          {
              pushParamsToJS();
          }))
{
    setSize (440, 520);
    addAndMakeVisible (webView);
    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    startTimerHz (30);
}

RoneStuckerAudioProcessorEditor::~RoneStuckerAudioProcessorEditor()
{
    stopTimer();
}

void RoneStuckerAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource> RoneStuckerAudioProcessorEditor::getResource (const juce::String& url)
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

void RoneStuckerAudioProcessorEditor::pushParamsToJS()
{
    auto* obj = new juce::DynamicObject();

    for (auto* id : { RoneParamIDs::AMOUNT, RoneParamIDs::SMOOTH,
                      RoneParamIDs::RANGE, RoneParamIDs::LOWCUT })
        if (auto* raw = processorRef.apvts.getRawParameterValue (id))
            obj->setProperty (id, raw->load());

    if (auto* raw = processorRef.apvts.getRawParameterValue (RoneParamIDs::LENGTH))
        obj->setProperty (RoneParamIDs::LENGTH, (int) raw->load());

    if (auto* raw = processorRef.apvts.getRawParameterValue (RoneParamIDs::BYPASS))
        obj->setProperty (RoneParamIDs::BYPASS, raw->load() > 0.5f);

    if (auto* raw = processorRef.apvts.getRawParameterValue (RoneParamIDs::SYNC))
        obj->setProperty (RoneParamIDs::SYNC, raw->load() > 0.5f);

    webView.emitEventIfBrowserIsVisible ("paramState", obj);
}

void RoneStuckerAudioProcessorEditor::timerCallback()
{
    // Keep the UI in sync with host automation + feed the depth meter
    pushParamsToJS();

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("depth",   processorRef.engine.getCurrentDepth());
    obj->setProperty ("engaged", processorRef.engine.isEngaged());
    obj->setProperty ("loopMs",  processorRef.engine.getCurrentLoopMs());
    webView.emitEventIfBrowserIsVisible ("engineState", obj);
}
