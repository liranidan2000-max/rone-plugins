#pragma once

#include <JuceHeader.h>

// ============================================================================
// AccountClient — email + password sign-in against roneaudio.com.
//
// This is the path customers use from 2026 on: they sign in with the same
// account they bought with, and the server answers with a device token plus
// what the account owns. The token is bound to this machine and is what the
// Center re-validates every 24h, so the password is typed once and never
// stored.
//
// It deliberately writes the SAME BundleLicense.xml that the older Lemon
// Squeezy key path writes, because every plugin already reads that file — the
// account system needed no plugin changes at all.
//
// The LS key path (LicenseHandler) stays in place as a fallback for customers
// who bought before accounts existed.
// ============================================================================
class AccountClient : private juce::Timer
{
public:
    AccountClient();
    ~AccountClient() override;

    struct State
    {
        bool signedIn = false;
        bool licensed = false;          // signed in AND the pass is active
        juce::String email;
        juce::String name;
        juce::String plan;              // "all-access" | "perpetual" | "none"
        juce::int64  expiresAt = 0;     // 0 = no expiry (perpetual)
        juce::int64  renewsAt  = 0;
        int deviceLimit = 2;
        juce::String message;
    };

    /** Loads the stored token and re-validates in the background. */
    void initialize();

    State getState() const;

    using Callback = std::function<void (bool success, juce::String message)>;

    void signIn  (const juce::String& email, const juce::String& password, Callback);

    /** Google: opens the browser on roneaudio.com, waits for it to hand a one-time
        code back on 127.0.0.1, exchanges it for a device token and finishes exactly
        like signIn(). Up to four minutes; cancelGoogleSignIn() aborts the wait. */
    void signInWithGoogle (Callback);
    void cancelGoogleSignIn();
    void signOut (Callback);

    /** Re-checks the token with the server; also called every 24h by the timer. */
    void validateAsync (std::function<void (bool licensed)> done = nullptr);

    /** Fired on the message thread whenever signedIn/licensed changes. */
    std::function<void()> onStateChanged;

    /** True when a token exists, regardless of whether it is still valid. */
    bool hasStoredToken() const;

private:
    // Same 7-day offline tolerance as the license-key path: a working studio
    // must not go dark because the internet did.
    static constexpr juce::int64 OFFLINE_GRACE_MS      = 7LL * 24 * 60 * 60 * 1000;
    static constexpr int         VALIDATION_INTERVAL_MS = 24 * 60 * 60 * 1000;

    juce::File getAccountFile() const;   // token + profile (Center only)
    juce::File getLicenseFile() const;   // the file every plugin reads

    void saveAccountFile();
    bool loadAccountFile();
    void clearAccountFile();

    /** Mirrors the current state into BundleLicense.xml (and the registry). */
    void writeBundleLicense (bool licensed);

    /** Stable per-machine id, hashed so the raw device id never leaves the PC. */
    static juce::String getMachineId();
    static juce::String getMachineName();

    juce::var post (const juce::String& path, const juce::var& body,
                    const juce::String& bearer, int& statusCodeOut);

    void applyServerState (const juce::var& response);
    void notifyChanged();

    void timerCallback() override;

    mutable juce::CriticalSection lock;
    State state;
    juce::String token;
    juce::int64 lastValidationTime = 0;

    // Guards against two overlapping network calls stomping on each other.
    std::atomic<bool> busy { false };
    std::atomic<bool> googleCancel { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AccountClient)
};
