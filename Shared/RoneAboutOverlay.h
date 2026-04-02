#pragma once

#ifdef _WIN32
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
#endif

// Shared/RoneAboutOverlay.h — Unified About overlay for all RONE plugins
// Included by: ReverseReverbVST, RoneStemsFixer, RoneStutter, rone-flanger-

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

//==============================================================================
/**
    Unified About overlay for all RONE plugins.
    Header-only — include via relative path from each plugin.

    Usage:
        RoneAboutOverlay aboutOverlay { "STUTTER", JucePlugin_VersionString };
        addChildComponent (aboutOverlay);
        // In resized():  aboutOverlay.setBounds (getLocalBounds());
        // To open:       aboutOverlay.show();
*/
class RoneAboutOverlay : public juce::Component,
                         private juce::Timer
{
public:
    //==========================================================================
    // Callbacks — set these for WebView plugins that need to hide/show the web view.
    std::function<void()> onShow;
    std::function<void()> onDismiss;

    //==========================================================================
    RoneAboutOverlay (const juce::String& pluginName,
                      const juce::String& versionString)
        : pluginName_ (pluginName),
          version_ (versionString)
    {
        setInterceptsMouseClicks (false, false);
        setVisible (false);

        // Website hyperlink
        websiteLink_.setButtonText ("RONEAUDIO.COM");
        websiteLink_.setURL (juce::URL ("https://roneaudio.com"));
        websiteLink_.setColour (juce::HyperlinkButton::textColourId, accent_);
        websiteLink_.setFont (juce::FontOptions (15.0f).withStyle ("Bold"), false);
        addChildComponent (websiteLink_);

        // Open Plugins Center button
        openCenterBtn_.setButtonText ("OPEN PLUGINS CENTER");
        openCenterBtn_.setColour (juce::TextButton::buttonColourId, accent_);
        openCenterBtn_.setColour (juce::TextButton::buttonOnColourId, accent_.brighter (0.15f));
        openCenterBtn_.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        openCenterBtn_.onClick = [this] { launchPluginsCenter(); };
        addChildComponent (openCenterBtn_);

        // Close "X" button
        closeBtn_.setButtonText (juce::CharPointer_UTF8 ("\xc3\x97")); // multiplication sign ×
        closeBtn_.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        closeBtn_.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff2a2a50));
        closeBtn_.setColour (juce::TextButton::textColourOffId, textSecondary_);
        closeBtn_.onClick = [this] { dismiss(); };
        addChildComponent (closeBtn_);
    }

    //==========================================================================
    void show()
    {
        showing_    = true;
        dismissing_ = false;
        animProgress_ = 0.0f;
        setVisible (true);
        toFront (true);
        setInterceptsMouseClicks (true, true);
        websiteLink_.setVisible (true);
        openCenterBtn_.setVisible (true);
        closeBtn_.setVisible (true);
        startTimerHz (60);

        if (onShow)
            onShow();
    }

    void dismiss()
    {
        dismissing_ = true;
        startTimerHz (60);
    }

    bool isShowing() const { return showing_; }

    //==========================================================================
    void resized() override
    {
        layoutPanel();
    }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        if (animProgress_ <= 0.0f)
            return;

        // Ease-out cubic
        float t = 1.0f - animProgress_;
        float eased = 1.0f - t * t * t;

        // Dimmed background
        g.setColour (dimBg_.withMultipliedAlpha (eased));
        g.fillAll();

        // Panel geometry
        auto pf = panelBounds_.toFloat();
        int slideOffset = static_cast<int> (20.0f * (1.0f - eased));
        pf.translate (0.0f, static_cast<float> (slideOffset));

        float alpha = eased;

        // Panel shadow
        g.setColour (juce::Colours::black.withAlpha (alpha * 0.45f));
        g.fillRoundedRectangle (pf.translated (0.0f, 5.0f), 16.0f);

        // Panel background gradient
        {
            juce::ColourGradient grad (panelBgTop_.brighter (0.08f),
                                        pf.getX(), pf.getY(),
                                        panelBg_,
                                        pf.getX(), pf.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (pf, 16.0f);
        }

        // Border
        g.setColour (accent_.withAlpha (alpha * 0.3f));
        g.drawRoundedRectangle (pf, 16.0f, 1.0f);

        // Content
        auto content = panelBounds_.reduced (32, 28);
        content.translate (0, slideOffset);

        // ---- Accent dot + RONE ----
        float dotSize = 14.0f;
        float dotX = static_cast<float> (content.getX());
        float dotY = static_cast<float> (content.getY()) + 6.0f;

        g.setColour (accent_.withAlpha (alpha * 0.25f));
        g.fillEllipse (dotX - 3.0f, dotY - 3.0f, dotSize + 6.0f, dotSize + 6.0f);
        g.setColour (accent_.withAlpha (alpha));
        g.fillEllipse (dotX, dotY, dotSize, dotSize);

        g.setColour (textPrimary_.withAlpha (alpha));
        g.setFont (juce::FontOptions (26.0f).withStyle ("Bold"));
        g.drawText ("RONE", static_cast<int> (dotX + dotSize + 8.0f), content.getY(),
                    200, 28, juce::Justification::centredLeft);

        // Plugin name subtitle
        g.setColour (accent_.withAlpha (alpha));
        g.setFont (juce::FontOptions (14.0f).withStyle ("Bold"));
        g.drawText (pluginName_, static_cast<int> (dotX + dotSize + 8.0f), content.getY() + 28,
                    200, 20, juce::Justification::centredLeft);

        // ---- Version pill ----
        {
            juce::String vText = "v" + version_;
            float pillW = juce::jmax (52.0f, (static_cast<float> (vText.length()) * 8.0f) + 16.0f);
            float pillX = dotX + dotSize + 210.0f;
            float pillY = static_cast<float> (content.getY()) + 30.0f;

            g.setColour (accent_.withAlpha (alpha * 0.15f));
            g.fillRoundedRectangle (pillX, pillY, pillW, 18.0f, 9.0f);
            g.setColour (accent_.withAlpha (alpha * 0.5f));
            g.drawRoundedRectangle (pillX, pillY, pillW, 18.0f, 9.0f, 1.0f);
            g.setColour (accent_.withAlpha (alpha * 0.8f));
            g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
            g.drawText (vText, static_cast<int> (pillX), static_cast<int> (pillY),
                        static_cast<int> (pillW), 18, juce::Justification::centred);
        }

        // ---- Divider ----
        float divY = static_cast<float> (content.getY()) + 64.0f;
        {
            juce::ColourGradient dg (accent_.withAlpha (0.0f), pf.getX() + 32.0f, divY,
                                      accent_.withAlpha (alpha * 0.3f), pf.getCentreX(), divY, false);
            dg.addColour (1.0, accent_.withAlpha (0.0f));
            g.setGradientFill (dg);
            g.fillRect (pf.getX() + 32.0f, divY, pf.getWidth() - 64.0f, 1.0f);
        }

        // ---- Description ----
        int descY = static_cast<int> (divY) + 18;
        g.setColour (textSecondary_.withAlpha (alpha));
        g.setFont (juce::FontOptions (13.0f));
        g.drawFittedText ("Part of the RONE audio plugin suite.\n"
                          "Designed for music producers and sound designers.",
                          content.getX(), descY, content.getWidth(), 40,
                          juce::Justification::centred, 2);

        // ---- Footer ----
        g.setColour (textMuted_.withAlpha (alpha * 0.5f));
        g.setFont (juce::FontOptions (10.0f));
        g.drawText ("RONE Plugins \xc2\xb7 2026",
                    content.getX(), panelBounds_.getBottom() + slideOffset - 48,
                    content.getWidth(), 18, juce::Justification::centred);

        // ---- Close hint ----
        g.setColour (textMuted_.withAlpha (alpha * 0.35f));
        g.setFont (juce::FontOptions (10.0f));
        g.drawText ("Click outside to close",
                    content.getX(), panelBounds_.getBottom() + slideOffset - 30,
                    content.getWidth(), 16, juce::Justification::centred);
    }

    //==========================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        // Click outside panel to dismiss
        if (! panelBounds_.contains (e.getPosition()))
            dismiss();
    }

    //==========================================================================
    /** Launch the RONE Plugins Center application. */
    static void launchPluginsCenter()
    {
       #if JUCE_MAC
        juce::File app ("/Applications/RONE Plugins Center.app");
        if (! app.exists())
            app = juce::File ("/Applications/RONE Plugins/RONE Plugins Center.app");
        if (app.exists())
            app.startAsProcess();
       #elif JUCE_WINDOWS
        auto app = juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory)
                       .getChildFile ("RONE Plugins")
                       .getChildFile ("RONE Plugins Center.exe");
        if (app.exists())
            app.startAsProcess();
       #endif
    }

private:
    //==========================================================================
    void timerCallback() override
    {
        if (dismissing_)
        {
            animProgress_ -= animSpeed_;
            if (animProgress_ <= 0.0f)
            {
                animProgress_ = 0.0f;
                showing_    = false;
                dismissing_ = false;
                setVisible (false);
                setInterceptsMouseClicks (false, false);
                websiteLink_.setVisible (false);
                openCenterBtn_.setVisible (false);
                closeBtn_.setVisible (false);
                stopTimer();

                if (onDismiss)
                    onDismiss();
            }
        }
        else
        {
            animProgress_ += animSpeed_;
            if (animProgress_ >= 1.0f)
            {
                animProgress_ = 1.0f;
                stopTimer();
            }
        }
        repaint();
    }

    //==========================================================================
    void layoutPanel()
    {
        auto bounds = getLocalBounds();
        int panelW = juce::jmin (420, (bounds.getWidth()  - 60));
        int panelH = juce::jmin (380, (bounds.getHeight() - 60));
        panelBounds_ = juce::Rectangle<int> (0, 0, panelW, panelH)
                           .withCentre (bounds.getCentre());

        auto content = panelBounds_.reduced (32, 28);

        // "Open Plugins Center" button — centered, below description
        int btnY = content.getY() + 140;
        int btnW = 220;
        int btnH = 36;
        openCenterBtn_.setBounds (content.getX() + (content.getWidth() - btnW) / 2,
                                   btnY, btnW, btnH);

        // Website hyperlink — below center button
        int linkW = 160;
        int linkH = 24;
        websiteLink_.setBounds (content.getX() + (content.getWidth() - linkW) / 2,
                                 btnY + btnH + 20, linkW, linkH);

        // Close "X" button — top-right of panel
        closeBtn_.setBounds (panelBounds_.getRight() - 40, panelBounds_.getY() + 8, 32, 32);
    }

    //==========================================================================
    juce::String pluginName_;
    juce::String version_;

    juce::HyperlinkButton websiteLink_;
    juce::TextButton      openCenterBtn_;
    juce::TextButton      closeBtn_;

    bool  showing_      = false;
    bool  dismissing_   = false;
    float animProgress_ = 0.0f;
    static constexpr float animSpeed_ = 0.07f;

    juce::Rectangle<int> panelBounds_;

    // Brand colours
    static inline const juce::Colour dimBg_          { 0xe6111114 };   // #111114 @ 0.9
    static inline const juce::Colour panelBg_        { 0xff1a1a2e };
    static inline const juce::Colour panelBgTop_     { 0xff141425 };
    static inline const juce::Colour accent_         { 0xff7c3aed };
    static inline const juce::Colour textPrimary_    { 0xffe8e8f0 };
    static inline const juce::Colour textSecondary_  { 0xff7a7a92 };
    static inline const juce::Colour textMuted_      { 0xff505068 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoneAboutOverlay)
};
