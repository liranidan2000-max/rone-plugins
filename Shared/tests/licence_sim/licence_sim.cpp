// licence_sim - the Shared licence chain, offline, without touching the real
// licence files (audit R004/R014/R182/R194/R279/R293).
//
//   cmake -S Shared/tests/licence_sim -B <build> -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR=<JUCE 8.0.4 source>
//   cmake --build <build> --config Release
//   <build>/licence_sim_artefacts/Release/licence_sim.exe     -> exit code 0 = every check passed
//   ...and the same with --config Debug (<build>/licence_sim_artefacts/Debug/licence_sim.exe).
//   Release alone is not enough: the debug CRT fills freed blocks with 0xDD, so a
//   pointer into a freed temporary fails the machine-id checks there instead of
//   passing by allocator luck (R014 review). An AddressSanitizer build is stricter still.
//
// Every file it writes lives in a temp folder it deletes again. It READS this
// computer's machine id and, if present, the real BundleLicense.xml, and
// prints only yes/no about them - never an id, a hash or a licence field.
// Signatures are made in-process with a fixed TEST seed (checked against
// Node's Ed25519 by make-vector.mjs), never with the production key.

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include "../../BundleLicenseChecker.h"
#include "../../RoneCrashReporter.h"

#include <cstdio>
#include <cstring>

static int failures = 0;
static void check (bool ok, const juce::String& what)
{
    std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
    if (! ok) ++failures;
}

// ---- friends of the classes under test ----
struct BundleLicenseCheckerTest
{
    using State = BundleLicenseChecker::LicenseState;
    static State parse (const juce::File& f, bool required, const unsigned char* pk)
        { return BundleLicenseChecker::parseLicenseFile (f, required, pk); }
    static State parseProductionKey (const juce::File& f, bool required)
        { return BundleLicenseChecker::parseLicenseFile (f, required); }
    static bool grace (juce::int64 lastValidation, juce::int64 graceMs)
        { State s; s.valid = true; s.lastValidation = lastValidation; return BundleLicenseChecker::withinGrace (s, graceMs); }
    static bool owns (const juce::String& products, const juce::String& id) { return BundleLicenseChecker::ownsProduct (products, id); }
    static bool otherMachine (const juce::String& m, const juce::StringArray& ids) { return BundleLicenseChecker::isOtherMachine (m, ids); }
    static constexpr juce::int64 bundleGrace    = BundleLicenseChecker::BUNDLE_GRACE_MS;
    static constexpr juce::int64 perpetualGrace = BundleLicenseChecker::PERPETUAL_GRACE_MS;
};

struct RemoteLicenseGateTest
{
    static bool stale (juce::int64 fetchedAt, juce::int64 attemptedAt, juce::int64 now)
        { return RemoteLicenseGate::isStale (fetchedAt, attemptedAt, now); }
    static bool write (const juce::File& f, juce::int64 fetchedAt, juce::int64 attemptedAt)
        { return RemoteLicenseGate::writeModeTo (f, "enforced", "msg", "required", fetchedAt, attemptedAt); }
    static RemoteLicenseGate::CachedMode read (const juce::File& f) { return RemoteLicenseGate::readModeFile (f); }
    static void observe (const juce::File& root, const juce::String& code)
        { RemoteLicenseGate::observeOnceIn (root, code, "test message", "host=test"); }
};

// ---- Ed25519 signing with TweetNaCl's crypto_sign (test seed only) ----
namespace testsign
{
    using namespace RoneEntitlement::detail;

    static void keypair (const u8 seed[32], u8 pk[32], u8 d[64])
    {
        gf p[4];
        sha512 (d, seed, 32);
        d[0] &= 248; d[31] &= 127; d[31] |= 64;
        scalarbase (p, d);
        pack (pk, p);
    }

    static void sign (u8 sigOut[64], const void* msg, size_t n, const u8 seed[32])
    {
        u8 d[64], pk[32], r[64], h[64];
        keypair (seed, pk, d);

        std::vector<u8> a (32 + n);
        std::memcpy (a.data(), d + 32, 32);
        if (n > 0) std::memcpy (a.data() + 32, msg, n);
        sha512 (r, a.data(), (u64) a.size());
        reduce (r);

        gf p[4];
        scalarbase (p, r);
        pack (sigOut, p);                                  // R

        std::vector<u8> b (64 + n);
        std::memcpy (b.data(), sigOut, 32);
        std::memcpy (b.data() + 32, pk, 32);
        if (n > 0) std::memcpy (b.data() + 64, msg, n);
        sha512 (h, b.data(), (u64) b.size());
        reduce (h);

        i64 x[64] = {};
        for (int i = 0; i < 32; ++i) x[i] = (i64) r[i];
        for (int i = 0; i < 32; ++i)
            for (int j = 0; j < 32; ++j)
                x[i + j] += (i64) h[i] * (i64) d[j];
        modL (sigOut + 32, x);                             // S
    }
}

static unsigned char kTestSeed[32];
static unsigned char kTestPk[32];

static juce::String b64 (const void* data, size_t n) { return juce::Base64::toBase64 (data, n); }

struct Signed { juce::String ent, sig; };

static Signed makeEntitlement (const juce::String& machine, bool pass, const juce::StringArray& owned, juce::int64 iat)
{
    juce::String ownedJson;
    for (auto& id : owned)
        ownedJson << (ownedJson.isEmpty() ? "" : ",") << "\"" << id << "\"";
    const auto json = "{\"v\":1,\"m\":\"" + machine + "\",\"pass\":" + (pass ? "true" : "false")
                    + ",\"owned\":[" + ownedJson + "],\"iat\":" + juce::String (iat) + "}";
    const auto utf8 = json.toUTF8();
    unsigned char sig[64];
    testsign::sign (sig, utf8.getAddress(), utf8.sizeInBytes() - 1, kTestSeed);
    return { b64 (utf8.getAddress(), utf8.sizeInBytes() - 1), b64 (sig, 64) };
}

static juce::File writeLicence (const juce::File& dir, const juce::String& name, const juce::StringPairArray& attrs)
{
    juce::XmlElement xml ("RoneBundleLicense");
    for (auto& k : attrs.getAllKeys())
        xml.setAttribute (k, attrs[k]);
    auto f = dir.getChildFile (name);
    xml.writeTo (f, {});
    return f;
}

static juce::String juceSha (const juce::String& s) { return juce::SHA256 (s.toUTF8()).toHexString(); }
static juce::String oursSha (const juce::String& s)
{
    const auto u = s.toUTF8();
    return RoneEntitlement::sha256Hex (u.getAddress(), u.sizeInBytes() - 1);
}

int main()
{
    const auto now = juce::Time::currentTimeMillis();
    const juce::int64 hour = 3600LL * 1000, day = 24 * hour;

    // ---------------------------------------------------------------- SHA-256
    check (oursSha ("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "sha256 of the empty string (FIPS 180-2)");
    check (oursSha ("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "sha256 of 'abc' (FIPS 180-2)");
    check (oursSha ("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")
               == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", "sha256 of the 448-bit FIPS message (two blocks)");
    {
        bool same = true;
        juce::Random rng (1234);
        for (int len = 0; len <= 300 && same; ++len)
        {
            juce::MemoryBlock mb ((size_t) len);
            for (int i = 0; i < len; ++i) static_cast<char*> (mb.getData())[i] = (char) rng.nextInt (256);
            same = RoneEntitlement::sha256Hex (mb.getData(), mb.getSize()) == juce::SHA256 (mb).toHexString();
        }
        check (same, "sha256 equals juce::SHA256 for every length 0..300 (all padding boundaries)");
    }
    check (oursSha ("rone-device:-6271043152829137185") == juceSha ("rone-device:-6271043152829137185"), "machine hash parity: Windows-style decimal device id");
    check (oursSha ("rone-device:4C4C4544004E3510804BB4C04F4E3732") == juceSha ("rone-device:4C4C4544004E3510804BB4C04F4E3732"), "machine hash parity: Mac IOPlatformUUID");
    check (oursSha (juce::String::fromUTF8 ("rone-device:\xd7\xa8\xd7\x95\xd7\x9f-\xc3\xa9")) == juceSha (juce::String::fromUTF8 ("rone-device:\xd7\xa8\xd7\x95\xd7\x9f-\xc3\xa9")), "machine hash parity: non-ASCII (UTF-8 bytes)");

    // ---------------------------------------------------- this computer's id
    {
        const auto device = juce::SystemStats::getUniqueDeviceID();
        const auto& ids = RoneEntitlement::localMachineIds();
        if (device.isNotEmpty())
            check (ids.size() >= 1 && ids[0] == juceSha ("rone-device:" + device),
                   "localMachineIds()[0] is the Center's AccountClient::getMachineId() for this computer");
        else
            std::printf ("INFO  this process sees no device id (machine-id.txt ids: %d)\n", ids.size());

        // The real file, read only: does the signed licence the Center wrote
        // here name this computer the way a plugin computes it? yes/no only.
        const auto real = BundleLicenseChecker::getLicenseFile();
        if (auto xml = juce::parseXML (real); xml != nullptr && xml->getStringAttribute ("ent").isNotEmpty())
        {
            if (auto p = RoneEntitlement::read (xml->getStringAttribute ("ent"), xml->getStringAttribute ("sig")))
                check (p->machine.isNotEmpty() && ids.contains (p->machine, true),
                       "this computer's real signed licence names this computer (plugin-side hash = server's m)");
            else
                std::printf ("INFO  the real BundleLicense.xml has an ent= that does not verify\n");
        }
        else
            std::printf ("INFO  no signed BundleLicense.xml on this computer - real-file parity not checked\n");
    }

    // ------------------------------------------------------- Ed25519 signer
    for (int i = 0; i < 32; ++i) kTestSeed[i] = (unsigned char) ((i * 7 + 3) & 0xff);
    {
        unsigned char d[64];
        testsign::keypair (kTestSeed, kTestPk, d);
        check (juce::String::toHexString (kTestPk, 32, 0) == "755c4cb9256ca7cdc4acfdc6cfeeda849017e5b9f9514e99191bd67e0b0d4276",
               "test public key equals Node's (make-vector.mjs)");
        const char* msg = "{\"v\":1,\"m\":\"abc\",\"pass\":true,\"owned\":[\"RoneThrow\"],\"iat\":1760000000000}";
        unsigned char sig[64];
        testsign::sign (sig, msg, std::strlen (msg), kTestSeed);
        check (juce::String::toHexString (sig, 64, 0)
                   == "7b9634836f52fb28ba0aa800f311b718b1de9d7c7b0fd700ee5c7fd32e2f959f57f9f6c76241ac1deb48f712838d112efdcf83788b65c9c1b715e7e128a1d102",
               "test signature equals Node's Ed25519 (make-vector.mjs)");
        check (RoneEntitlement::verify (msg, std::strlen (msg), sig, kTestPk), "RoneEntitlement::verify accepts Node's signature");
        sig[10] ^= 1;
        check (! RoneEntitlement::verify (msg, std::strlen (msg), sig, kTestPk), "RoneEntitlement::verify rejects one flipped bit");
    }

    // ------------------------------------------------- parseLicenseFile cases
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                   .getChildFile ("rone-licence-sim-" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt64()));
    dir.createDirectory();
    using T = BundleLicenseCheckerTest;
    {
        juce::StringPairArray a;
        a.set ("licensed", "1"); a.set ("lastValidationTime", juce::String (now - hour));
        const auto f = writeLicence (dir, "plain.xml", a);
        const auto off = T::parse (f, false, kTestPk), on = T::parse (f, true, kTestPk);
        check (off.valid && off.bundle && T::grace (off.lastValidation, T::bundleGrace), "plain ALL ACCESS file counts while signed licences are not required (today)");
        check (! on.valid, "plain file claims nothing once signed licences are required");
    }
    {
        juce::StringPairArray a;
        a.set ("licensed", "0"); a.set ("products", "RoneThrow, RoneStutter"); a.set ("lastValidationTime", juce::String (now - hour));
        const auto f = writeLicence (dir, "plain-owned.xml", a);
        const auto s = T::parse (f, false, kTestPk);
        check (s.valid && ! s.bundle && T::owns (s.products, "RoneStutter") && ! T::owns (s.products, "RoneStucker"), "plain lifetime file: owned list read whole-token");
    }
    {
        const auto e = makeEntitlement ("", true, {}, now - hour);
        juce::StringPairArray a;
        a.set ("licensed", "1"); a.set ("lastValidationTime", juce::String (now)); a.set ("ent", e.ent); a.set ("sig", e.sig);
        const auto f = writeLicence (dir, "signed-pass.xml", a);
        const auto off = T::parse (f, false, kTestPk), on = T::parse (f, true, kTestPk);
        check (off.valid && off.bundle && off.lastValidation == now - hour, "signed pass: valid, ALL ACCESS, stamp = signed iat (not the plain stamp)");
        check (on.valid && on.bundle, "signed pass still counts once signed licences are required");
        check (! T::parseProductionKey (f, false).valid, "a test-key signature does not verify against the production key");
    }
    {
        auto e = makeEntitlement ("", true, {}, now);
        auto chars = e.ent.toStdString();
        chars[chars.size() / 2] = chars[chars.size() / 2] == 'A' ? 'B' : 'A';
        juce::StringPairArray a;
        a.set ("licensed", "1"); a.set ("lastValidationTime", juce::String (now)); a.set ("ent", juce::String (chars)); a.set ("sig", e.sig);
        const auto f = writeLicence (dir, "tampered.xml", a);
        check (! T::parse (f, false, kTestPk).valid, "edited ent= claims nothing, even beside licensed=1");
    }
    {
        const auto e = makeEntitlement ("", false, juce::StringArray { "RoneThrow" }, now - hour);
        juce::StringPairArray a;
        a.set ("serialLicensed", "1"); a.set ("lastValidationTime", juce::String (now - hour)); a.set ("ent", e.ent); a.set ("sig", e.sig);
        const auto f = writeLicence (dir, "signed-serial.xml", a);
        const auto off = T::parse (f, false, kTestPk), on = T::parse (f, true, kTestPk);
        check (off.valid && off.bundle && T::owns (off.products, "RoneThrow"), "Lemon Squeezy serial beside a signed lifetime licence counts while not required");
        check (on.valid && ! on.bundle && T::owns (on.products, "RoneThrow"), "once required: the serial no longer counts, the signed lifetime licence does");
    }
    {
        // R182: a slow PC clock - the server's iat lands in the local future.
        const auto e1 = makeEntitlement ("", true, {}, now + 30 * 60 * 1000);
        juce::StringPairArray a; a.set ("ent", e1.ent); a.set ("sig", e1.sig);
        const auto s1 = T::parse (writeLicence (dir, "future-30min.xml", a), false, kTestPk);
        check (s1.valid && s1.lastValidation >= now && s1.lastValidation <= juce::Time::currentTimeMillis()
                   && T::grace (s1.lastValidation, T::bundleGrace),
               "signed iat 30 min in the local future counts as now (slow clock, R182)");
        const auto e2 = makeEntitlement ("", true, {}, now + 2 * day);
        a.set ("ent", e2.ent); a.set ("sig", e2.sig);
        const auto s2 = T::parse (writeLicence (dir, "future-2d.xml", a), false, kTestPk);
        check (s2.valid && ! T::grace (s2.lastValidation, T::bundleGrace), "signed iat 2 days ahead still fails closed (clock wound back)");
    }

    // --------------------------------------------------------- grace windows
    check (T::grace (now - 7 * day, T::bundleGrace) && ! T::grace (now - 9 * day, T::bundleGrace), "ALL ACCESS grace is 8 days");
    check (T::grace (now - 89 * day, T::perpetualGrace) && ! T::grace (now - 91 * day, T::perpetualGrace), "lifetime grace is 90 days");
    check (T::grace (now + 2 * 60 * 1000, T::bundleGrace) && ! T::grace (now + 10 * 60 * 1000, T::bundleGrace), "plain stamp: 5 min NTP slack, no more");
    check (! T::grace (0, T::bundleGrace), "no stamp, no licence");
    check (! T::owns ("RoneStutter", "RoneStut") && T::owns (" RoneStucker , X", "ronestucker"), "product match is whole-token and case-insensitive");

    // ------------------------------------------------- R014 machine compare
    {
        const juce::StringArray ids { "aa11", "bb22" };
        check (! T::otherMachine ("", ids), "no machine in the payload: no difference");
        check (! T::otherMachine ("cc33", {}), "this computer's id unknown: no difference");
        check (! T::otherMachine ("aa11", ids) && ! T::otherMachine ("BB22", ids), "either local id matches, any case");
        check (T::otherMachine ("cc33", ids), "another computer's id: a difference (report only)");
    }

    // ------------------------------------------- R279 refresh stamp / retry
    using G = RemoteLicenseGateTest;
    check (! G::stale (now - hour, 0, now), "manifest 1 h old: fresh");
    check (G::stale (now - 7 * hour, 0, now), "manifest 7 h old, no try recorded: due");
    check (! G::stale (now - 7 * hour, now - 5 * 60 * 1000, now), "due, but a try 5 min ago: wait");
    check (G::stale (now - 7 * hour, now - 11 * 60 * 1000, now), "due, last try 11 min ago: retry (was 6 h)");
    check (G::stale (now + hour, 0, now), "manifest stamped in the future: due (clock moved back)");
    check (G::stale (now - 7 * hour, now + hour, now), "a try stamped in the future holds nothing back");
    {
        const auto f = dir.getChildFile ("LicenseMode.xml");
        check (G::write (f, 123, 456), "writeModeTo reports a successful write");
        const auto c = G::read (f);
        check (c.mode == "enforced" && c.message == "msg" && c.signedLicences == "required" && c.fetchedAt == 123 && c.attemptedAt == 456,
               "mode file round trip keeps mode, message, signed, fetchedAt and attemptedAt");
        G::write (f, 789, 0);
        check (G::read (f).attemptedAt == 0, "a fresh manifest (the Center's write) carries no attemptedAt");
    }

    // ----------------------------------------------- observation (temp root)
    {
        const auto root = dir.getChildFile ("CrashReports");
        G::observe (root, "LICENCE_TEST");
        G::observe (root, "LICENCE_TEST");
        const auto files = root.getChildFile ("pending").findChildFiles (juce::File::findFiles, false, "*.json");
        check (files.size() == 1, "an observation is queued once, the second within 30 days is dropped");
        if (files.size() == 1)
        {
            const auto j = juce::JSON::parse (files[0]);
            bool keys = true;
            for (auto* k : { "type", "product", "version", "wrapper", "code", "message", "details", "stack", "os", "arch", "locale", "install_id", "time" })
                keys = keys && j.hasProperty (k);
            check (keys && j["type"].toString() == "error" && j["code"].toString() == "LICENCE_TEST"
                       && j["install_id"].toString().length() == 36,
                   "the observation has every field the site's crash intake reads");
        }
    }

    // ------------------------------------------------------- R293 arch label
   #if JUCE_WINDOWS && JUCE_INTEL && JUCE_64BIT
    check (RoneCrashReporter::runningArch() == "x64", "crash reports from 64-bit Windows still say x64");
   #else
    std::printf ("INFO  runningArch() = %s\n", RoneCrashReporter::runningArch().toRawUTF8());
   #endif

    dir.deleteRecursively();
    std::printf ("\n%s (%d failed)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
