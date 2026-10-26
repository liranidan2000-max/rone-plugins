#pragma once

// Serves the RONE UI fonts (Shared/fonts, linked by rone_add_fonts() in
// Shared/fonts/RoneFonts.cmake) to the WebView page, so it renders in Sora /
// Manrope / IBM Plex Mono with no network. In the editor's getResource():
//
//     if (auto font = roneFontResource (url))
//         return font;
//
// The page asks for them with Shared/fonts/rone-fonts.css (url('fonts/<file>')).

#include <juce_gui_extra/juce_gui_extra.h>
#include "RoneFontsData.h"

inline std::optional<juce::WebBrowserComponent::Resource> roneFontResource (const juce::String& url)
{
    if (! url.startsWith ("/fonts/"))
        return std::nullopt;

    const auto file = url.fromLastOccurrenceOf ("/", false, false);
    for (int i = 0; i < RoneFontsData::namedResourceListSize; ++i)
    {
        if (file != RoneFontsData::originalFilenames[i])
            continue;

        int size = 0;
        if (auto* data = reinterpret_cast<const std::byte*> (RoneFontsData::getNamedResource (RoneFontsData::namedResourceList[i], size)))
            return juce::WebBrowserComponent::Resource { std::vector<std::byte> (data, data + size), juce::String ("font/woff2") };
    }
    return std::nullopt;
}
