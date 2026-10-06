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
// That file now also carries `products=` — the plugins bought outright as a
// one-off LIFETIME licence. `licensed=` keeps meaning "the ALL ACCESS pass is
// active" and nothing else, so already-shipped builds, which only read
// `licensed`, stay locked for a lifetime-only customer. That is deliberate.
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
        // Canonical ids of the plugins bought outright (LIFETIME). Independent
        // of `licensed`: an account can own plugins and hold no pass at all.
        juce::StringArray ownedProducts;
        // Who bills the LIVE pass, exactly as the server names it: "paddle",
        // "lemonsqueezy" or "comp" (a pass Liran gave by hand). Empty when there
        // is no live pass, a giveaway trial, or nothing heard yet (an account
        // file written before 1.5.1). A gift holder is never sold the pass.
        juce::String passSource;
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
    // must not go dark because the internet did. It gates the PASS only — a
    // lifetime plugin was paid for outright and has nothing to expire, so it
    // stays written however long we have been unable to reach the server.
    static constexpr juce::int64 OFFLINE_GRACE_MS      = 7LL * 24 * 60 * 60 * 1000;
    static constexpr int         VALIDATION_INTERVAL_MS = 24 * 60 * 60 * 1000;

    juce::File getAccountFile() const;   // token + profile (Center only)

    void saveAccountFile();
    bool loadAccountFile();
    void clearAccountFile();

    /** Mirrors this account's two attributes into BundleLicense.xml through
        BundleLicenseFile: licensed="1" only for an active pass, products="..."
        for the plugins bought outright. The serial path's claim on the same
        file is left exactly as it was found, and the file is deleted only once
        NEITHER side holds anything — that absence is what every plugin reads
        as "not licensed".

        The lastValidationTime it hands over is the last time the SERVER
        answered, not the moment of writing. Every plugin's offline grace is
        measured from it, so it must only ever move forward in
        applyServerState(); a write from an offline path that refreshed it
        would make a refund impossible to enforce. */
    void writeLicenseFile();

    /** Drops the account's claim on BundleLicense.xml. A legacy Lemon Squeezy
        serial on the same machine survives it. */
    void clearLicenseFile();

    /** Stable per-machine id, hashed so the raw device id never leaves the PC. */
    static juce::String getMachineId();
    static juce::String getMachineName();

    juce::var post (const juce::String& path, const juce::var& body,
                    const juce::String& bearer, int& statusCodeOut);

    void applyServerState (const juce::var& response);
    void notifyChanged();
    void postToMessageThread (std::function<void()> fn);

    // Counts the network threads that are still using this object.
    struct InFlight
    {
        explicit InFlight (std::atomic<int>& c) : count (c) { ++count; }
        ~InFlight() { --count; }
        std::atomic<int>& count;
    };
    std::atomic<int> inFlight { 0 };
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);

    void timerCallback() override;

    mutable juce::CriticalSection lock;
    State state;
    juce::String token;
    juce::int64 lastValidationTime = 0;

    // The server's signed copy of the entitlements (Center 2.0), written into
    // BundleLicense.xml untouched. Empty from a server that does not sign.
    juce::String signedEntitlement, signedEntitlementSig;

    // Guards against two overlapping network calls stomping on each other.
    std::atomic<bool> busy { false };

    // One /app/refresh at a time: the window coming to the front, the start-up
    // check and the daily timer can all ask, and each run rewrites the account
    // file and BundleLicense.xml.
    std::atomic<bool> validating { false };
    std::atomic<bool> googleCancel { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AccountClient)
};
