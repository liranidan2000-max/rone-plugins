#include "MainComponent.h"

// ============================================================================
// Construction / Destruction
// ============================================================================

MainComponent::MainComponent()
{
    setLookAndFeel (&roneLnf);

    // --- Title ---
    titleLabel.setText ("RONE PLUGINS CENTER", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, Colours_RONE::hotPurple);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    // --- Status ---
    statusLabel.setFont (juce::FontOptions (11.5f));
    statusLabel.setColour (juce::Label::textColourId, Colours_RONE::textSecondary);
    statusLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (statusLabel);

    // --- Refresh ---
    refreshButton.setColour (juce::TextButton::buttonColourId, Colours_RONE::buttonBase);
    refreshButton.onClick = [this] { refreshPlugins(); };
    addAndMakeVisible (refreshButton);

    // --- Viewport + card container ---
    viewport.setViewedComponent (&cardContainer, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    networkManager.addListener (this);

    // --- License UI ---
    licenseKeyInput.setMultiLine (false);
    licenseKeyInput.setTextToShowWhenEmpty ("Enter your RONE Full Bundle license key...",
                                            Colours_RONE::textDim);
    licenseKeyInput.setColour (juce::TextEditor::backgroundColourId,  Colours_RONE::licenseBg);
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

    licenseStatusLabel.setFont (juce::FontOptions (11.0f));
    licenseStatusLabel.setColour (juce::Label::textColourId, Colours_RONE::textSecondary);
    addAndMakeVisible (licenseStatusLabel);

    proBadge.setText ("PRO", juce::dontSendNotification);
    proBadge.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    proBadge.setColour (juce::Label::textColourId, Colours_RONE::neonPink);
    proBadge.setJustificationType (juce::Justification::centred);
    proBadge.setVisible (false);
    addAndMakeVisible (proBadge);

    // --- Init license ---
    licenseHandler.onLicenseStateChanged = [this] (bool) { updateLicenseUI(); };
    licenseHandler.initialize();
    updateLicenseUI();

    // --- Initial fetch ---
    statusLabel.setText ("Checking for updates...", juce::dontSendNotification);
    refreshPlugins();

    // --- Horizontal layout: wider, shorter ---
    setSize (840, 520);

    // --- Fade-in animation: start transparent, animate over 400ms ---
    setAlpha (0.0f);
    fadeAlpha = 0.0f;
    fadeComplete = false;
    startTimerHz (60); // 60 FPS for smooth animation
}

MainComponent::~MainComponent()
{
    stopTimer();
    networkManager.removeListener (this);
    setLookAndFeel (nullptr);
}

// ============================================================================
// Timer — handles fade-in animation + auto-refresh
// ============================================================================

void MainComponent::timerCallback()
{
    if (! fadeComplete)
    {
        fadeAlpha += 0.05f; // ~300ms total at 60fps
        if (fadeAlpha >= 1.0f)
        {
            fadeAlpha = 1.0f;
            fadeComplete = true;
            stopTimer();

            // Restart as 30-minute refresh timer
            startTimer (30 * 60 * 1000);
        }
        setAlpha (fadeAlpha);
    }
    else
    {
        // Auto-refresh
        refreshPlugins();
    }
}

// ============================================================================
// Paint — header + license bar + background
// ============================================================================

void MainComponent::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (Colours_RONE::background);

    // Header — 48px
    auto headerArea = getLocalBounds().removeFromTop (48);
    {
        juce::ColourGradient grad (Colours_RONE::headerBg,
                                    (float) headerArea.getX(), (float) headerArea.getY(),
                                    Colours_RONE::headerBg.brighter (0.06f),
                                    (float) headerArea.getX(), (float) headerArea.getBottom(),
                                    false);
        g.setGradientFill (grad);
        g.fillRect (headerArea);
    }

    // Thin glow line
    auto glowY = headerArea.getBottom();
    g.setColour (Colours_RONE::hotPurple.withAlpha (0.35f));
    g.fillRect (headerArea.getX(), glowY, headerArea.getWidth(), 1);

    // License bar — 46px
    auto licenseBar = getLocalBounds();
    licenseBar.removeFromTop (49);
    auto lBar = licenseBar.removeFromTop (46);
    g.setColour (Colours_RONE::licenseBg);
    g.fillRect (lBar);

    // Separator
    g.setColour (Colours_RONE::cardBorder.withAlpha (0.4f));
    g.fillRect (lBar.getX(), lBar.getBottom(), lBar.getWidth(), 1);
}

// ============================================================================
// Layout — 2-column grid
// ============================================================================

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // Header: 48px
    auto header = area.removeFromTop (48).reduced (20, 8);
    refreshButton.setBounds (header.removeFromRight (70));
    header.removeFromRight (10);
    statusLabel.setBounds (header.removeFromRight (180));
    titleLabel.setBounds (header);

    // License bar: 46px
    area.removeFromTop (1); // glow line
    auto licenseArea = area.removeFromTop (46).reduced (20, 7);

    if (licenseHandler.isLicensed())
    {
        auto row = licenseArea.removeFromTop (28);
        proBadge.setBounds (row.removeFromLeft (38));
        row.removeFromLeft (6);
        deactivateButton.setBounds (row.removeFromRight (88));
        row.removeFromRight (8);
        licenseStatusLabel.setBounds (row);

        licenseKeyInput.setVisible (false);
        activateButton.setVisible (false);
    }
    else
    {
        auto row1 = licenseArea.removeFromTop (26);
        activateButton.setBounds (row1.removeFromRight (88));
        row1.removeFromRight (8);
        licenseKeyInput.setBounds (row1);

        licenseArea.removeFromTop (2);
        licenseStatusLabel.setBounds (licenseArea);

        proBadge.setVisible (false);
        deactivateButton.setVisible (false);
    }

    // Card grid area
    area.removeFromTop (6);
    viewport.setBounds (area.reduced (8, 0));

    // Layout cards in 2-column grid
    const int numCards = cards.size();
    if (numCards == 0) return;

    const int cardGap = 10;
    const int cols    = 2;
    const int containerW = viewport.getWidth() - viewport.getScrollBarThickness() - 4;
    const int cardW   = (containerW - cardGap * (cols + 1)) / cols;
    const int cardH   = 155;

    int rows = (numCards + cols - 1) / cols;
    int totalH = rows * (cardH + cardGap) + cardGap;

    cardContainer.setBounds (0, 0, containerW, totalH);

    for (int i = 0; i < numCards; ++i)
    {
        int col = i % cols;
        int row = i / cols;
        int x = cardGap + col * (cardW + cardGap);
        int y = cardGap + row * (cardH + cardGap);

        cards[i]->setBounds (x, y, cardW, cardH);
    }
}

// ============================================================================
// Refresh
// ============================================================================

void MainComponent::refreshPlugins()
{
    statusLabel.setText ("Checking for updates...", juce::dontSendNotification);
    networkManager.fetchManifest();
}

// ============================================================================
// NetworkManager callbacks
// ============================================================================

void MainComponent::onManifestReady (const juce::Array<PluginInfo>& plugins)
{
    pluginData = plugins;
    cards.clear();

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

    // Apply license state
    refreshCardLicenseState();

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
    statusLabel.setText ("Offline \u2014 " + errorMessage, juce::dontSendNotification);
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
    if (! licenseHandler.isLicensed()) return;

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
    if (! licenseHandler.isLicensed()) return;

    for (auto& p : pluginData)
    {
        if (p.id == pluginId)
        {
        #if JUCE_MAC
            if (p.standaloneExe.isNotEmpty())
            {
                auto appName = p.standaloneExe.replace (".exe", "") + ".app";
                auto app = juce::File ("/Applications").getChildFile (appName);
                if (! app.exists())
                    app = VersionChecker::getStandaloneInstallDir().getChildFile (appName);

                if (app.exists())
                {
                    app.startAsProcess();
                    break;
                }
            }

            // No standalone found — offer to install it
            {
                auto pid = p.id;
                auto name = p.name;
                juce::NativeMessageBox::showYesNoBox (
                    juce::MessageBoxIconType::QuestionIcon,
                    "Standalone Not Installed",
                    name + " is installed as a plugin (VST3/AU) only.\n\n"
                    "Download and install the standalone version?",
                    nullptr,
                    juce::ModalCallbackFunction::create ([this, pid] (int result)
                    {
                        if (result == 1) // Yes
                        {
                            for (auto& pl : pluginData)
                            {
                                if (pl.id == pid)
                                {
                                    pl.status = PluginStatus::Downloading;
                                    pl.downloadProgress = 0.0;
                                    if (auto* card = findCard (pid))
                                        card->setDownloadProgress (0.0);
                                #if JUCE_MAC
                                    networkManager.downloadInstaller (pid, pl.downloadUrlMac);
                                #else
                                    networkManager.downloadInstaller (pid, pl.downloadUrl);
                                #endif
                                    break;
                                }
                            }
                        }
                    }));
            }
        #else
            if (p.standaloneExe.isNotEmpty())
            {
                auto exe = VersionChecker::getStandaloneInstallDir()
                               .getChildFile (p.standaloneExe);
                if (exe.existsAsFile())
                {
                    exe.startAsProcess();
                    break;
                }
            }

            juce::NativeMessageBox::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon,
                "Not Found",
                "Could not find " + p.standaloneExe + " on disk.\n"
                "Try reinstalling the plugin.");
        #endif
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
                p.name + " \u2014 Info", msg);
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

    auto filePath = installerFile.getFullPathName();
    auto pid      = pluginId;

    juce::String regKey, remoteVer, vst3Bundle, auBundle, standaloneExe;

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
        juce::String cmd = juce::String ("osascript -e 'do shell script \"installer -pkg ")
                         + "\\\"" + filePath + "\\\""
                         + " -target /\" with administrator privileges'";
        started = process.start (cmd);
    #else
        juce::String cmd = "\"" + filePath + "\" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-";
        started = process.start (cmd);
    #endif

        bool processFinished = false;
        if (started)
            processFinished = process.waitForProcessToFinish (120000);

        bool verified = false;

    #if JUCE_MAC
        if (processFinished)
        {
            verified = VersionChecker::isVst3Installed (vst3Bundle)
                    || VersionChecker::isAUInstalled (auBundle)
                    || VersionChecker::isStandaloneInstalled (standaloneExe);
            if (verified)
                VersionChecker::setInstalledVersion (regKey, remoteVer);
        }
    #else
        if (processFinished)
        {
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
                        if (p.installedVersion.isEmpty())
                            p.installedVersion = remoteVer;
                        p.status = PluginStatus::UpToDate;
                        statusLabel.setText ("Installed successfully!",
                                              juce::dontSendNotification);

                        // Auto-open standalone after install if available
                    #if JUCE_MAC
                        if (p.standaloneExe.isNotEmpty())
                        {
                            auto appName = p.standaloneExe.replace (".exe", "") + ".app";
                            auto app = juce::File ("/Applications").getChildFile (appName);
                            if (! app.exists())
                                app = VersionChecker::getStandaloneInstallDir().getChildFile (appName);
                            if (app.exists())
                                app.startAsProcess();
                            else
                                juce::NativeMessageBox::showMessageBoxAsync (
                                    juce::MessageBoxIconType::WarningIcon,
                                    "Standalone Not Found",
                                    "The plugin was installed but the standalone app was not found.\n"
                                    "The VST3/AU plugin is ready to use in your DAW.");
                        }
                    #endif
                    }
                    else
                    {
                        p.status = PluginStatus::Error;
                    #if JUCE_MAC
                        statusLabel.setText ("Install failed \u2014 check your password and try again.",
                                              juce::dontSendNotification);
                    #else
                        statusLabel.setText ("Install failed \u2014 try running as Administrator.",
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
        if (success)
        {
            updateLicenseUI();
            statusLabel.setText ("License activated \u2014 all plugins unlocked!",
                                  juce::dontSendNotification);
        }
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

    // Show "Licensed to [customer name]" when licensed
    if (pro)
    {
        auto name = licenseHandler.getCustomerName();
        if (name.isNotEmpty())
            licenseStatusLabel.setText ("Licensed to " + name + " \u2014 RONE Full Bundle",
                                        juce::dontSendNotification);
        else
            licenseStatusLabel.setText ("Licensed \u2014 RONE Full Bundle",
                                        juce::dontSendNotification);
    }
    else
    {
        licenseStatusLabel.setText (licenseHandler.getStatusMessage(),
                                    juce::dontSendNotification);
    }

    refreshCardLicenseState();
    resized();
}

void MainComponent::refreshCardLicenseState()
{
    bool pro = licenseHandler.isLicensed();
    for (auto* card : cards)
        card->setLicensed (pro);
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
