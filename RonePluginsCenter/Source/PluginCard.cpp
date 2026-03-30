#include "PluginCard.h"
#include "BinaryData.h"

PluginCard::PluginCard()
{
    nameLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    nameLabel.setColour (juce::Label::textColourId, Colours_RONE::textPrimary);
    addAndMakeVisible (nameLabel);

    descLabel.setFont (juce::FontOptions (11.5f));
    descLabel.setColour (juce::Label::textColourId, Colours_RONE::textSecondary);
    addAndMakeVisible (descLabel);

    versionLabel.setFont (juce::FontOptions (10.5f));
    versionLabel.setColour (juce::Label::textColourId, Colours_RONE::textDim);
    addAndMakeVisible (versionLabel);

    actionButton.addListener (this);
    addAndMakeVisible (actionButton);

    openButton.addListener (this);
    openButton.setColour (juce::TextButton::buttonColourId, Colours_RONE::buttonBase);
    addAndMakeVisible (openButton);

    infoButton.addListener (this);
    infoButton.setColour (juce::TextButton::buttonColourId,
                           Colours_RONE::cardBorder.brighter (0.2f));
    addAndMakeVisible (infoButton);

    progressBar.setVisible (false);
    addAndMakeVisible (progressBar);
}

// ============================================================================
// Load the correct logo from embedded binary data
// ============================================================================

void PluginCard::loadPluginLogo()
{
    const char* data = nullptr;
    int size = 0;

    if (pluginInfo.id == "ReverseReverb")
    {
        data = BinaryData::ReverseReverb_icon_png;
        size = BinaryData::ReverseReverb_icon_pngSize;
    }
    else if (pluginInfo.id == "RoneStutter")
    {
        data = BinaryData::RoneStutter_icon_png;
        size = BinaryData::RoneStutter_icon_pngSize;
    }
    else if (pluginInfo.id == "RoneStemsFixer")
    {
        data = BinaryData::RoneStemsFixer_icon_png;
        size = BinaryData::RoneStemsFixer_icon_pngSize;
    }
    else if (pluginInfo.id == "RoneFlanger")
    {
        data = BinaryData::RoneFlanger_icon_png;
        size = BinaryData::RoneFlanger_icon_pngSize;
    }

    if (data != nullptr && size > 0)
        pluginLogo = juce::ImageFileFormat::loadFrom (data, (size_t) size);
}

void PluginCard::setPluginInfo (const PluginInfo& info)
{
    pluginInfo = info;

    nameLabel.setText (info.name, juce::dontSendNotification);
    descLabel.setText (info.description, juce::dontSendNotification);

    juce::String verText;
    if (info.installedVersion.isNotEmpty() && info.installedVersion != "?")
        verText = "v" + info.installedVersion;
    else
        verText = "v" + info.remoteVersion;

    if (info.status == PluginStatus::UpdateAvailable)
        verText += "  \xe2\x86\x92  v" + info.remoteVersion;  // →

    versionLabel.setText (verText, juce::dontSendNotification);

    if (! pluginLogo.isValid())
        loadPluginLogo();

    updateButtonState();
    repaint();
}

void PluginCard::setDownloadProgress (double progress)
{
    pluginInfo.downloadProgress = progress;
    pluginInfo.status = PluginStatus::Downloading;
    progressBar.setVisible (true);
    actionButton.setEnabled (false);
    actionButton.setButtonText ("DOWNLOADING...");
    repaint();
}

void PluginCard::setLicensed (bool isLicensed)
{
    licensed = isLicensed;
    updateButtonState();
    repaint();
}

void PluginCard::updateButtonState()
{
    progressBar.setVisible (false);
    actionButton.setEnabled (true);
    openButton.setVisible (false);

    switch (pluginInfo.status)
    {
        case PluginStatus::NotInstalled:
            actionButton.setButtonText ("INSTALL");
            actionButton.setColour (juce::TextButton::buttonColourId,
                                     Colours_RONE::hotPurple);
            break;

        case PluginStatus::UpdateAvailable:
            actionButton.setButtonText ("UPDATE");
            actionButton.setColour (juce::TextButton::buttonColourId,
                                     Colours_RONE::neonPink);
            break;

        case PluginStatus::UpToDate:
            actionButton.setButtonText ("Installed");
            actionButton.setColour (juce::TextButton::buttonColourId,
                                     Colours_RONE::installedGreen.withAlpha (0.18f));
            actionButton.setEnabled (false);
            openButton.setVisible (true);
            break;

        case PluginStatus::Downloading:
            actionButton.setButtonText ("DOWNLOADING...");
            actionButton.setEnabled (false);
            progressBar.setVisible (true);
            break;

        case PluginStatus::Installing:
            actionButton.setButtonText ("INSTALLING...");
            actionButton.setEnabled (false);
            break;

        case PluginStatus::Error:
            actionButton.setButtonText ("RETRY");
            actionButton.setColour (juce::TextButton::buttonColourId,
                                     Colours_RONE::errorRed);
            break;
    }

    // ---- License paywall ----
    if (! licensed)
    {
        actionButton.setEnabled (false);
        actionButton.setAlpha (0.35f);
        openButton.setEnabled (false);
        openButton.setAlpha (0.35f);
        actionButton.setTooltip ("Activate your license to install RONE plugins.");
        openButton.setTooltip ("Activate your license to open RONE plugins.");
    }
    else
    {
        actionButton.setAlpha (1.0f);
        openButton.setAlpha (1.0f);
        openButton.setEnabled (true);
        actionButton.setTooltip ({});
        openButton.setTooltip ({});
    }
}

// ============================================================================
// Draw format badges
// ============================================================================

void PluginCard::drawFormatBadges (juce::Graphics& g, juce::Rectangle<int> area)
{
    int x = area.getX();
    int y = area.getY();
    int h = area.getHeight();

    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));

    for (auto& fmt : pluginInfo.formats)
    {
        auto text = fmt.toUpperCase();
        int textW = (int) g.getCurrentFont().getStringWidthFloat (text) + 14;

        auto pill = juce::Rectangle<float> ((float) x, (float) y,
                                             (float) textW, (float) h);

        juce::Colour pillCol = Colours_RONE::badgeStandalone;
        if (text == "VST3")    pillCol = Colours_RONE::badgeVST3;
        else if (text == "AU") pillCol = Colours_RONE::badgeAU;

        g.setColour (pillCol.withAlpha (0.25f));
        g.fillRoundedRectangle (pill, 4.0f);

        g.setColour (pillCol.brighter (0.5f).withAlpha (0.6f));
        g.drawRoundedRectangle (pill, 4.0f, 0.8f);

        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.drawText (text, pill.toNearestInt(), juce::Justification::centred);

        x += textW + 5;
    }
}

// ============================================================================
// Paint — clean card with logo, gradient, Apple-style depth
// ============================================================================

void PluginCard::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);

    // Drop shadow (Apple-style elevation)
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (bounds.translated (0, 2).expanded (1), 14.0f);

    // Card fill — subtle gradient
    {
        juce::ColourGradient grad (Colours_RONE::cardBackground.brighter (0.06f),
                                    bounds.getX(), bounds.getY(),
                                    Colours_RONE::cardBackground.darker (0.05f),
                                    bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, 12.0f);
    }

    // Border
    juce::Colour borderCol = Colours_RONE::cardBorder;
    switch (pluginInfo.status)
    {
        case PluginStatus::UpToDate:        borderCol = Colours_RONE::installedGreen.withAlpha (0.2f);  break;
        case PluginStatus::UpdateAvailable: borderCol = Colours_RONE::neonPink.withAlpha (0.3f);       break;
        case PluginStatus::Downloading:     borderCol = Colours_RONE::hotPurple.withAlpha (0.4f);      break;
        case PluginStatus::Error:           borderCol = Colours_RONE::errorRed.withAlpha (0.4f);       break;
        default: break;
    }

    g.setColour (borderCol);
    g.drawRoundedRectangle (bounds, 12.0f, 1.0f);

    // --- Plugin logo (left side) ---
    auto contentArea = getLocalBounds().reduced (16, 12);
    auto logoArea = contentArea.removeFromLeft (52);

    if (pluginLogo.isValid())
    {
        auto logoBounds = logoArea.withSizeKeepingCentre (48, 48).toFloat();

        // Soft glow behind logo
        g.setColour (Colours_RONE::hotPurple.withAlpha (0.08f));
        g.fillRoundedRectangle (logoBounds.expanded (4), 12.0f);

        g.drawImage (pluginLogo, logoBounds,
                     juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }
    else
    {
        // Fallback: letter circle
        auto circle = logoArea.withSizeKeepingCentre (48, 48).toFloat();
        g.setColour (Colours_RONE::hotPurple.withAlpha (0.3f));
        g.fillEllipse (circle);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        g.drawText (pluginInfo.name.substring (0, 2).toUpperCase(),
                    circle.toNearestInt(), juce::Justification::centred);
    }

    // --- Format badges ---
    contentArea.removeFromLeft (12); // gap after logo
    auto badgeY = contentArea.getY() + 46;
    drawFormatBadges (g, juce::Rectangle<int> (contentArea.getX(), badgeY,
                                                contentArea.getWidth(), 17));

    // --- Lock badge when unlicensed ---
    if (! licensed)
    {
        auto lockBadge = getLocalBounds().reduced (16, 0)
                             .removeFromBottom (28).removeFromRight (70)
                             .translated (0, -8);

        g.setColour (Colours_RONE::errorRed.withAlpha (0.12f));
        g.fillRoundedRectangle (lockBadge.toFloat(), 6.0f);

        g.setColour (Colours_RONE::errorRed.withAlpha (0.65f));
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawText ("\xf0\x9f\x94\x92 LOCKED", lockBadge, juce::Justification::centred);
    }
}

// ============================================================================
// Layout
// ============================================================================

void PluginCard::resized()
{
    auto area = getLocalBounds().reduced (16, 12);

    // Logo space
    area.removeFromLeft (64); // 52 logo + 12 gap

    // Info button top-right
    auto topRow = area.removeFromTop (20);
    infoButton.setBounds (topRow.removeFromRight (22).reduced (0, 0));
    topRow.removeFromRight (4);
    nameLabel.setBounds (topRow);

    area.removeFromTop (2);
    descLabel.setBounds (area.removeFromTop (15));

    // Badges space (drawn in paint)
    area.removeFromTop (28);

    // Version
    versionLabel.setBounds (area.removeFromTop (14));

    area.removeFromTop (3);

    // Progress bar
    if (progressBar.isVisible())
    {
        progressBar.setBounds (area.removeFromTop (6));
        area.removeFromTop (3);
    }

    // Buttons
    auto buttonRow = area.removeFromBottom (26);
    actionButton.setBounds (buttonRow.removeFromLeft (100));

    if (openButton.isVisible())
    {
        buttonRow.removeFromLeft (6);
        openButton.setBounds (buttonRow.removeFromLeft (56));
    }
}

// ============================================================================
// Button callbacks
// ============================================================================

void PluginCard::buttonClicked (juce::Button* button)
{
    if (button == &actionButton && onActionClicked)
    {
        if (licensed)
            onActionClicked (pluginInfo.id);
    }
    else if (button == &openButton && onOpenClicked)
    {
        if (licensed)
            onOpenClicked (pluginInfo.id);
    }
    else if (button == &infoButton && onInfoClicked)
    {
        onInfoClicked (pluginInfo.id);
    }
}
