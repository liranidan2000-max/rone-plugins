#include "PluginCard.h"

PluginCard::PluginCard()
{
    // Name label — large, bold
    nameLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    nameLabel.setColour (juce::Label::textColourId, Colours_RONE::textPrimary);
    addAndMakeVisible (nameLabel);

    // Description label — smaller, dimmer
    descLabel.setFont (juce::FontOptions (13.0f));
    descLabel.setColour (juce::Label::textColourId, Colours_RONE::textSecondary);
    addAndMakeVisible (descLabel);

    // Version label
    versionLabel.setFont (juce::FontOptions (12.0f));
    versionLabel.setColour (juce::Label::textColourId, Colours_RONE::textDim);
    addAndMakeVisible (versionLabel);

    // Formats label (VST3, Standalone, etc.)
    formatsLabel.setFont (juce::FontOptions (11.0f));
    formatsLabel.setColour (juce::Label::textColourId, Colours_RONE::neonPurple);
    addAndMakeVisible (formatsLabel);

    // Action button
    actionButton.addListener (this);
    addAndMakeVisible (actionButton);

    // Open button (for standalones)
    openButton.addListener (this);
    openButton.setColour (juce::TextButton::buttonColourId, Colours_RONE::cardBorder);
    addAndMakeVisible (openButton);

    // Info button
    infoButton.addListener (this);
    infoButton.setColour (juce::TextButton::buttonColourId, Colours_RONE::cardBorder);
    addAndMakeVisible (infoButton);

    // Progress bar (hidden by default)
    progressBar.setVisible (false);
    addAndMakeVisible (progressBar);
}

void PluginCard::setPluginInfo (const PluginInfo& info)
{
    pluginInfo = info;

    nameLabel.setText (info.name, juce::dontSendNotification);
    descLabel.setText (info.description, juce::dontSendNotification);
    formatsLabel.setText (info.formats.joinIntoString (" | "),
                          juce::dontSendNotification);

    // Version string
    juce::String verText;
    if (info.installedVersion.isNotEmpty() && info.installedVersion != "?")
        verText = "v" + info.installedVersion;
    else
        verText = "v" + info.remoteVersion;

    if (info.status == PluginStatus::UpdateAvailable)
        verText += "  ->  v" + info.remoteVersion;

    versionLabel.setText (verText, juce::dontSendNotification);

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
                                     Colours_RONE::neonBlue);
            break;

        case PluginStatus::UpdateAvailable:
            actionButton.setButtonText ("UPDATE");
            actionButton.setColour (juce::TextButton::buttonColourId,
                                     Colours_RONE::neonOrange);
            break;

        case PluginStatus::UpToDate:
            actionButton.setButtonText ("INSTALLED");
            actionButton.setColour (juce::TextButton::buttonColourId,
                                     Colours_RONE::neonGreen.withAlpha (0.25f));
            actionButton.setEnabled (false);

            // Show "Open" for standalones
            if (pluginInfo.standaloneExe.isNotEmpty())
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
                                     Colours_RONE::neonPink);
            break;
    }
}

// ============================================================================
// Paint — draws the card background + glow border
// ============================================================================

void PluginCard::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);

    // Card fill
    g.setColour (Colours_RONE::cardBackground);
    g.fillRoundedRectangle (bounds, 10.0f);

    // Border — colour varies by status
    juce::Colour borderCol = Colours_RONE::cardBorder;

    switch (pluginInfo.status)
    {
        case PluginStatus::UpToDate:       borderCol = Colours_RONE::neonGreen.withAlpha (0.3f);  break;
        case PluginStatus::UpdateAvailable: borderCol = Colours_RONE::neonOrange.withAlpha (0.4f); break;
        case PluginStatus::Downloading:    borderCol = Colours_RONE::neonBlue.withAlpha (0.5f);   break;
        case PluginStatus::Error:          borderCol = Colours_RONE::neonPink.withAlpha (0.5f);   break;
        default: break;
    }

    g.setColour (borderCol);
    g.drawRoundedRectangle (bounds, 10.0f, 1.5f);

    // Top accent line (neon glow strip)
    auto accent = bounds.removeFromTop (3.0f).reduced (20.0f, 0);
    g.setColour (borderCol.withAlpha (0.7f));
    g.fillRoundedRectangle (accent, 1.5f);
}

// ============================================================================
// Layout
// ============================================================================

void PluginCard::resized()
{
    auto area = getLocalBounds().reduced (16, 12);

    auto topRow = area.removeFromTop (26);
    nameLabel.setBounds (topRow.removeFromLeft (topRow.getWidth() - 30));
    infoButton.setBounds (topRow.reduced (2));

    area.removeFromTop (2);
    descLabel.setBounds (area.removeFromTop (18));

    area.removeFromTop (4);
    formatsLabel.setBounds (area.removeFromTop (16));

    area.removeFromTop (2);
    versionLabel.setBounds (area.removeFromTop (16));

    area.removeFromTop (8);

    // Progress bar (full width if visible)
    if (progressBar.isVisible())
    {
        progressBar.setBounds (area.removeFromTop (10));
        area.removeFromTop (8);
    }

    // Buttons at the bottom
    auto buttonRow = area.removeFromBottom (32);
    int buttonW = 120;

    actionButton.setBounds (buttonRow.removeFromLeft (buttonW));

    if (openButton.isVisible())
    {
        buttonRow.removeFromLeft (8);
        openButton.setBounds (buttonRow.removeFromLeft (70));
    }
}

// ============================================================================
// Button callbacks
// ============================================================================

void PluginCard::buttonClicked (juce::Button* button)
{
    if (button == &actionButton && onActionClicked)
        onActionClicked (pluginInfo.id);

    else if (button == &openButton && onOpenClicked)
        onOpenClicked (pluginInfo.id);

    else if (button == &infoButton && onInfoClicked)
        onInfoClicked (pluginInfo.id);
}
