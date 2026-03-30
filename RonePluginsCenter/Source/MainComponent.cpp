#include "MainComponent.h"

// ============================================================================
// Construction / Destruction
// ============================================================================

MainComponent::MainComponent()
{
    setLookAndFeel (&roneLnf);

    // --- Title label (top bar) ---
    titleLabel.setText ("RONE PLUGINS CENTER", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, Colours_RONE::hotPurple);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    // --- Status label ---
    statusLabel.setFont (juce::FontOptions (13.0f));
    statusLabel.setColour (juce::Label::textColourId, Colours_RONE::textSecondary);
    statusLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (statusLabel);

    // --- Refresh button ---
    refreshButton.setColour (juce::TextButton::buttonColourId, Colours_RONE::buttonBase);
    refreshButton.onClick = [this] { refreshPlugins(); };
    addAndMakeVisible (refreshButton);

    // --- Scrollable viewport ---
    viewport.setViewedComponent (&cardContainer, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    // --- Register as network listener ---
    networkManager.addListener (this);

    // --- License section ---
    licenseKeyInput.setMultiLine (false);
    licenseKeyInput.setTextToShowWhenEmpty ("Enter license key...", Colours_RONE::textDim);
    licenseKeyInput.setColour (juce::TextEditor::backgroundColourId,  Colours_RONE::cardBackground);
    licenseKeyInput.setColour (juce::TextEditor::textColourId,        Colours_RONE::textPrimary);
    licenseKeyInput.setColour (juce::TextEditor::outlineColourId,     Colours_RONE::cardBorder);
    licenseKeyInput.setColour (juce::TextEditor::focusedOutlineColourId, Colours_RONE::hotPurple);
    addAndMakeVisible (licenseKeyInput);

    activateButton.setColour (juce::TextButton::buttonColourId, Colours_RONE::hotPurple);
    activateButton.onClick = [this] { handleActivate(); };
    addAndMakeVisible (activateButton);

    deactivateButton.setColour (juce::TextButton::buttonColourId, Colours_RONE::buttonBase);
    deactivateButton.onClick = [this] { handleDeactivate(); };
    deactivateButton.setVisible (false);
    addAndMakeVisible (deactivateButton);

    licenseStatusLabel.setFont (juce::FontOptions (12.0f));
    licenseStatusLabel.setColour (juce::Label::textColourId, Colours_RONE::textSecondary);
    addAndMakeVisible (licenseStatusLabel);

    proBadge.setText ("PRO", juce::dontSendNotification);
    proBadge.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    proBadge.setColour (juce::Label::textColourId, Colours_RONE::neonPink);
    proBadge.setJustificationType (juce::Justification::centred);
    proBadge.setVisible (false);
    addAndMakeVisible (proBadge);

    // --- Initialize license ---
    licenseHandler.onLicenseStateChanged = [this] (bool) { updateLicenseUI(); };
    licenseHandler.initialize();
    updateLicenseUI();

    // --- Initial fetch ---
    statusLabel.setText ("Checking for updates...", juce::dontSendNotification);
    refreshPlugins();

    // --- Auto-refresh every 30 minutes ---
    startTimer (30 * 60 * 1000);

    setSize (520, 730);
}

MainComponent::~MainComponent()
{
    stopTimer();
    networkManager.removeListener (this);
    setLookAndFeel (nullptr);
}

// ============================================================================
// Paint — background gradient
// ============================================================================

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (Colours_RONE::background);

    // Header bar — gradient fill
    auto headerArea = getLocalBounds().removeFromTop (56);
    {
        juce::ColourGradient headerGrad (Colours_RONE::headerBg,
                                          (float) headerArea.getX(), (float) headerArea.getY(),
                                          Colours_RONE::headerBg.brighter (0.08f),
                                          (float) headerArea.getX(), (float) headerArea.getBottom(),
                                          false);
        g.setGradientFill (headerGrad);
        g.fillRect (headerArea);
    }

    // Decorative glow line under header (2px + 2px bloom)
    auto glowY = headerArea.getBottom() - 1;
    g.setColour (Colours_RONE::hotPurple.withAlpha (0.5f));
    g.fillRect (headerArea.getX(), glowY, headerArea.getWidth(), 2);

    g.setColour (Colours_RONE::hotPurple.withAlpha (0.15f));
    g.fillRect (headerArea.getX(), glowY - 2, headerArea.getWidth(), 2);
}

// ============================================================================
// Layout
// ============================================================================

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // Header: 56 px tall
    auto header = area.removeFromTop (56).reduced (16, 10);
    refreshButton.setBounds (header.removeFromRight (80));
    header.removeFromRight (8);
    statusLabel.setBounds (header.removeFromRight (200));
    titleLabel.setBounds (header);

    // License section: 44 px tall
    auto licenseArea = area.removeFromTop (44).reduced (16, 4);

    if (licenseHandler.isLicensed())
    {
        proBadge.setBounds (licenseArea.removeFromLeft (40));
        licenseArea.removeFromLeft (8);
        licenseStatusLabel.setBounds (licenseArea.removeFromLeft (240));
        deactivateButton.setBounds (licenseArea.removeFromRight (100));
    }
    else
    {
        licenseKeyInput.setBounds (licenseArea.removeFromLeft (260));
        licenseArea.removeFromLeft (8);
        activateButton.setBounds (licenseArea.removeFromLeft (90));
        licenseArea.removeFromLeft (8);
        licenseStatusLabel.setBounds (licenseArea);
    }

    // Viewport takes the rest
    area.removeFromTop (8);
    viewport.setBounds (area.reduced (8, 0));

    // Layout cards inside the container
    const int cardH      = 180;
    const int cardGap    = 12;
    const int totalH     = cards.size() * (cardH + cardGap) + cardGap;
    const int containerW = viewport.getWidth() - viewport.getScrollBarThickness() - 4;

    cardContainer.setBounds (0, 0, containerW, totalH);

    int y = cardGap;
    for (auto* card : cards)
    {
        card->setBounds (cardGap, y, containerW - 2 * cardGap, cardH);
        y += cardH + cardGap;
    }
}

// ============================================================================
// Refresh — trigger manifest fetch
// ============================================================================

void MainComponent::refreshPlugins()
{
    statusLabel.setText ("Checking for updates...", juce::dontSendNotification);
    networkManager.fetchManifest();
}

void MainComponent::timerCallback()
{
    refreshPlugins();
}

// ============================================================================
// NetworkManager callbacks
// ============================================================================

void MainComponent::onManifestReady (const juce::Array<PluginInfo>& plugins)
{
    pluginData = plugins;
    cards.clear();

    // Safety net — if even the fallback returned nothing
    if (plugins.isEmpty())
    {
        statusLabel.setText ("Could not load plugins \u2014 check your connection",
                              juce::dontSendNotification);
        resized();
        return;
    }

    for (auto& info : pluginData)
    {
        auto* card = cards.add (new PluginCard());
        card->setPluginInfo (info);

        card->onActionClicked = [this] (const juce::String& id) { handleAction (id); };
        card->onOpenClicked   = [this] (const juce::String& id) { handleOpen (id); };
        card->onInfoClicked   = [this] (const juce::String& id) { handleInfo (id); };

        cardContainer.addAndMakeVisible (card);
    }

    int updatesAvailable = 0;
    for (auto& p : pluginData)
        if (p.status == PluginStatus::UpdateAvailable || p.status == PluginStatus::NotInstalled)
            ++updatesAvailable;

    if (updatesAvailable > 0)
        statusLabel.setText (juce::String (updatesAvailable) + " update(s) available",
                              juce::dontSendNotification);
    else
        statusLabel.setText ("All plugins up to date", juce::dontSendNotification);

    resized();
}

void MainComponent::onManifestError (const juce::String& errorMessage)
{
    statusLabel.setText ("Offline — " + errorMessage, juce::dontSendNotification);
}

void MainComponent::onDownloadProgress (const juce::String& pluginId, double progress)
{
    if (auto* card = findCard (pluginId))
        card->setDownloadProgress (progress);
}

void MainComponent::onDownloadComplete (const juce::String& pluginId,
                                         const juce::File& localFile,
                                         bool success,
                                         const juce::String& errorMessage)
{
    if (success)
    {
        launchSilentInstaller (localFile, pluginId);
    }
    else
    {
        // Set error state on the card
        for (auto& p : pluginData)
        {
            if (p.id == pluginId)
            {
                p.status = PluginStatus::Error;
                if (auto* card = findCard (pluginId))
                    card->setPluginInfo (p);
                break;
            }
        }

        juce::NativeMessageBox::showMessageBoxAsync (
            juce::MessageBoxIconType::WarningIcon,
            "Download Failed", errorMessage);
    }
}

// ============================================================================
// Action handlers
// ============================================================================

void MainComponent::handleAction (const juce::String& pluginId)
{
    for (auto& p : pluginData)
    {
        if (p.id == pluginId)
        {
            if (p.status == PluginStatus::NotInstalled
             || p.status == PluginStatus::UpdateAvailable
             || p.status == PluginStatus::Error)
            {
                p.status = PluginStatus::Downloading;
                p.downloadProgress = 0.0;

                if (auto* card = findCard (pluginId))
                    card->setDownloadProgress (0.0);

            #if JUCE_MAC
                networkManager.downloadInstaller (pluginId, p.downloadUrlMac);
            #else
                networkManager.downloadInstaller (pluginId, p.downloadUrl);
            #endif
            }
            break;
        }
    }
}

void MainComponent::handleOpen (const juce::String& pluginId)
{
    for (auto& p : pluginData)
    {
        if (p.id == pluginId && p.standaloneExe.isNotEmpty())
        {
            auto exe = VersionChecker::getStandaloneInstallDir()
                           .getChildFile (p.standaloneExe);
            if (exe.existsAsFile())
                exe.startAsProcess();
            else
                juce::NativeMessageBox::showMessageBoxAsync (
                    juce::MessageBoxIconType::WarningIcon,
                    "Not Found",
                    "Could not find " + p.standaloneExe + " on disk.\n"
                    "Try reinstalling the plugin.");
            break;
        }
    }
}

void MainComponent::handleInfo (const juce::String& pluginId)
{
    for (auto& p : pluginData)
    {
        if (p.id == pluginId)
        {
            juce::String msg;
            msg << p.name << " v" << p.remoteVersion << "\n\n"
                << "What's New:\n" << p.whatsNew << "\n\n"
                << "Formats: " << p.formats.joinIntoString (", ") << "\n"
                << "Type: " << p.type;

            if (p.installedVersion.isNotEmpty())
                msg << "\nInstalled: v" + p.installedVersion;

            juce::NativeMessageBox::showMessageBoxAsync (
                juce::MessageBoxIconType::InfoIcon,
                p.name + " — Changelog", msg);
            break;
        }
    }
}

// ============================================================================
// Silent installer execution
// ============================================================================

void MainComponent::launchSilentInstaller (const juce::File& installerFile,
                                            const juce::String& pluginId)
{
    // Update card state
    for (auto& p : pluginData)
    {
        if (p.id == pluginId)
        {
            p.status = PluginStatus::Installing;
            if (auto* card = findCard (pluginId))
                card->setPluginInfo (p);
            break;
        }
    }

    statusLabel.setText ("Installing...", juce::dontSendNotification);

    // Gather info before launching the background thread
    auto filePath = installerFile.getFullPathName();
    auto pid      = pluginId;

    juce::String regKey;
    juce::String remoteVer;
    juce::String vst3Bundle;
    juce::String auBundle;
    juce::String standaloneExe;

    for (auto& p : pluginData)
    {
        if (p.id == pluginId)
        {
            regKey        = p.registryKey;
            remoteVer     = p.remoteVersion;
            vst3Bundle    = p.vst3Bundle;
            auBundle      = p.auBundle;
            standaloneExe = p.standaloneExe;
            break;
        }
    }

    juce::Thread::launch ([this, filePath, pid, regKey, remoteVer,
                           vst3Bundle, auBundle, standaloneExe]
    {
        juce::ChildProcess process;
        bool started = false;

    #if JUCE_MAC
        // macOS: use 'installer' command via osascript for admin privileges.
        // The .pkg installs VST3/AU to /Library/Audio/Plug-Ins/ system-wide.
        juce::String cmd = juce::String ("osascript -e 'do shell script \"installer -pkg ")
                         + "\\\"" + filePath + "\\\""
                         + " -target /\" with administrator privileges'";
        started = process.start (cmd);
    #else
        // Windows: Inno Setup silent flags
        juce::String cmd = "\"" + filePath + "\" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-";
        started = process.start (cmd);
    #endif

        bool processFinished = false;
        if (started)
            processFinished = process.waitForProcessToFinish (120000); // 2 min timeout

        // Verify installation success
        bool verified = false;

    #if JUCE_MAC
        if (processFinished)
        {
            // On Mac, verify by checking if the plugin files appeared on disk
            verified = VersionChecker::isVst3Installed (vst3Bundle)
                    || VersionChecker::isAUInstalled (auBundle)
                    || VersionChecker::isStandaloneInstalled (standaloneExe);

            // If verified, write the version to our shared XML tracker
            if (verified)
                VersionChecker::setInstalledVersion (regKey, remoteVer);
        }
    #else
        if (processFinished)
        {
            // On Windows, the Inno Setup installer writes the registry key
            auto installedVer = VersionChecker::getInstalledVersion (regKey);
            verified = installedVer.isNotEmpty();
        }
    #endif

        juce::MessageManager::callAsync ([this, pid, verified, remoteVer]
        {
            for (auto& p : pluginData)
            {
                if (p.id == pid)
                {
                    if (verified)
                    {
                        p.installedVersion = VersionChecker::getInstalledVersion (p.registryKey);

                        // On Mac the XML was just written with remoteVer;
                        // if getInstalledVersion returned empty, use remoteVer directly
                        if (p.installedVersion.isEmpty())
                            p.installedVersion = remoteVer;

                        p.status = PluginStatus::UpToDate;
                        statusLabel.setText ("Installed successfully!",
                                              juce::dontSendNotification);
                    }
                    else
                    {
                        p.status = PluginStatus::Error;
                    #if JUCE_MAC
                        statusLabel.setText ("Install failed — check your password and try again.",
                                              juce::dontSendNotification);
                    #else
                        statusLabel.setText ("Install failed — try running as Administrator.",
                                              juce::dontSendNotification);
                    #endif
                    }

                    if (auto* card = findCard (pid))
                        card->setPluginInfo (p);
                    break;
                }
            }
        });
    });
}

// ============================================================================
// License handlers
// ============================================================================

void MainComponent::handleActivate()
{
    auto key = licenseKeyInput.getText().trim();
    if (key.isEmpty()) return;

    activateButton.setEnabled (false);
    licenseStatusLabel.setText ("Activating...", juce::dontSendNotification);

    licenseHandler.activateLicense (key, [this] (bool success, juce::String msg)
    {
        activateButton.setEnabled (true);
        licenseStatusLabel.setText (msg, juce::dontSendNotification);
        if (success) updateLicenseUI();
    });
}

void MainComponent::handleDeactivate()
{
    licenseHandler.deactivateLicense ([this] (bool success, juce::String msg)
    {
        licenseStatusLabel.setText (msg, juce::dontSendNotification);
        if (success) updateLicenseUI();
    });
}

void MainComponent::updateLicenseUI()
{
    bool pro = licenseHandler.isLicensed();

    proBadge.setVisible (pro);
    licenseKeyInput.setVisible (! pro);
    activateButton.setVisible (! pro);
    deactivateButton.setVisible (pro);

    licenseStatusLabel.setText (licenseHandler.getStatusMessage(),
                                juce::dontSendNotification);
    resized();
}

// ============================================================================
// Helpers
// ============================================================================

PluginCard* MainComponent::findCard (const juce::String& pluginId)
{
    for (int i = 0; i < pluginData.size(); ++i)
        if (pluginData[i].id == pluginId)
            return cards[i];

    return nullptr;
}
