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
    titleLabel.setColour (juce::Label::textColourId, Colours_RONE::neonGreen);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    // --- Status label ---
    statusLabel.setFont (juce::FontOptions (13.0f));
    statusLabel.setColour (juce::Label::textColourId, Colours_RONE::textSecondary);
    statusLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (statusLabel);

    // --- Refresh button ---
    refreshButton.setColour (juce::TextButton::buttonColourId, Colours_RONE::cardBorder);
    refreshButton.onClick = [this] { refreshPlugins(); };
    addAndMakeVisible (refreshButton);

    // --- Scrollable viewport ---
    viewport.setViewedComponent (&cardContainer, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    // --- Register as network listener ---
    networkManager.addListener (this);

    // --- Initial fetch ---
    statusLabel.setText ("Checking for updates...", juce::dontSendNotification);
    refreshPlugins();

    // --- Auto-refresh every 30 minutes ---
    startTimer (30 * 60 * 1000);

    setSize (520, 680);
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

    // Header bar background
    auto headerArea = getLocalBounds().removeFromTop (56);
    g.setColour (Colours_RONE::headerBg);
    g.fillRect (headerArea);

    // Bottom glow line under header
    g.setColour (Colours_RONE::neonPurple.withAlpha (0.3f));
    g.fillRect (headerArea.getX(), headerArea.getBottom() - 1,
                headerArea.getWidth(), 1);
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

                networkManager.downloadInstaller (pluginId, p.downloadUrl);
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

void MainComponent::launchSilentInstaller (const juce::File& installerExe,
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

    // Launch installer in background thread
    auto exePath = installerExe.getFullPathName();
    auto pid     = pluginId;

    // Find the registry key and remote version before launching the thread
    juce::String regKey;
    juce::String remoteVer;
    for (auto& p : pluginData)
    {
        if (p.id == pluginId)
        {
            regKey    = p.registryKey;
            remoteVer = p.remoteVersion;
            break;
        }
    }

    juce::Thread::launch ([this, exePath, pid, regKey, remoteVer]
    {
        juce::ChildProcess process;
        // Inno Setup silent flags — individual plugin installer
        bool started = process.start (
            "\"" + exePath + "\" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-");

        bool processFinished = false;
        if (started)
            processFinished = process.waitForProcessToFinish (120000); // 2 min timeout

        // Verify success by checking if the installer wrote the registry key.
        // This is more reliable than just checking the process exit —
        // the individual plugin installer writes InstalledVersion on success.
        bool verified = false;
        if (processFinished)
        {
            auto installedVer = VersionChecker::getInstalledVersion (regKey);
            verified = installedVer.isNotEmpty();
        }

        juce::MessageManager::callAsync ([this, pid, verified, remoteVer]
        {
            for (auto& p : pluginData)
            {
                if (p.id == pid)
                {
                    if (verified)
                    {
                        // The individual installer already wrote the registry key.
                        // Read the actual installed version back from it.
                        p.installedVersion = VersionChecker::getInstalledVersion (p.registryKey);
                        p.status = PluginStatus::UpToDate;
                        statusLabel.setText ("Installed successfully!",
                                              juce::dontSendNotification);
                    }
                    else
                    {
                        p.status = PluginStatus::Error;
                        statusLabel.setText ("Install failed — try running as Administrator.",
                                              juce::dontSendNotification);
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
// Helpers
// ============================================================================

PluginCard* MainComponent::findCard (const juce::String& pluginId)
{
    for (int i = 0; i < pluginData.size(); ++i)
        if (pluginData[i].id == pluginId)
            return cards[i];

    return nullptr;
}
