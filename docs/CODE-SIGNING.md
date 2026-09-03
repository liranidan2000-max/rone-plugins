# Code signing for RONE releases

Every installer the pipeline publishes can be signed automatically. The
workflow already contains the steps; they stay dormant until the matching
GitHub secrets exist, so nothing changes until you add them. Add the secrets
under **GitHub → rone-plugins → Settings → Secrets and variables → Actions →
New repository secret**. Names are exact.

| Platform | Script | What gets signed |
|---|---|---|
| Windows | `scripts/ci/sign-windows.ps1` | every `.exe` / `.dll` / VST3 binary, the uninstaller, `Setup.exe` |
| macOS | `scripts/ci/sign-macos.sh` | `.vst3` / `.component` / `.app` bundles, the `.pkg` (productsign), the Center `.dmg`; then Apple notarization + stapling |

Keep every private file (`.p12`, `.p8`, `.pfx`, keys) in
`D:\RONE PLUGINS\rone-secrets\`, never in the repo or in chat.

---

## macOS — Apple Developer ID (required for Gatekeeper)

Unsigned or un-notarized software is blocked on macOS by default ("cannot be
opened because the developer cannot be verified"). Only Apple can issue the
certificate; there is no alternative vendor.

### 1. Enroll (once, $99 / year)

<https://developer.apple.com/programs/enroll/> — *individual* enrollment is
fine (no company needed). Use an Apple ID with two-factor authentication.
Apple verifies your identity; that can take a few days.

### 2. Create the two Developer ID certificates (no Mac needed)

You need a key pair and a certificate request. Git's OpenSSL on Windows does it
(run in `D:\RONE PLUGINS\rone-secrets\`):

```bash
"C:\Program Files\Git\mingw64\bin\openssl.exe" genrsa -out apple-devid.key 2048
"C:\Program Files\Git\mingw64\bin\openssl.exe" req -new -key apple-devid.key -out apple-devid.csr -subj "/emailAddress=liranidan2000@gmail.com/CN=Liran Rone Kalifa/C=IL"
```

Then at <https://developer.apple.com/account/resources/certificates/add>:

1. Choose **Developer ID Application** → upload `apple-devid.csr` → download the `.cer` (call it `devid-application.cer`).
2. Repeat with **Developer ID Installer** using the same `.csr` → `devid-installer.cer`.

Combine both certificates with the key into one `.p12` (pick a password, you
will store it as a secret):

```bash
"C:\Program Files\Git\mingw64\bin\openssl.exe" x509 -inform der -in devid-application.cer -out devid-application.pem
"C:\Program Files\Git\mingw64\bin\openssl.exe" x509 -inform der -in devid-installer.cer -out devid-installer.pem
cat devid-application.pem devid-installer.pem > devid-both.pem
"C:\Program Files\Git\mingw64\bin\openssl.exe" pkcs12 -export -inkey apple-devid.key -in devid-both.pem -out apple-devid.p12 -name "RONE Developer ID"
```

### 3. Create an App Store Connect API key (for notarization)

<https://appstoreconnect.apple.com/access/integrations/api> → **Team Keys** →
**Generate API Key**, name `rone-ci`, access **Developer**. Note the **Key ID**
and the **Issuer ID** shown on that page, and download the `AuthKey_XXXX.p8`
(it can be downloaded only once).

### 4. Add the secrets

PowerShell one-liners to produce the base64 values:

```powershell
[Convert]::ToBase64String([IO.File]::ReadAllBytes("D:\RONE PLUGINS\rone-secrets\apple-devid.p12")) | Set-Clipboard
[Convert]::ToBase64String([IO.File]::ReadAllBytes("D:\RONE PLUGINS\rone-secrets\AuthKey_XXXX.p8")) | Set-Clipboard
```

| Secret | Value |
|---|---|
| `MAC_CERT_P12_BASE64` | base64 of `apple-devid.p12` |
| `MAC_CERT_PASSWORD` | the password you chose for the `.p12` |
| `APPLE_API_KEY_ID` | the Key ID (10 characters) |
| `APPLE_API_ISSUER_ID` | the Issuer ID (UUID) |
| `APPLE_API_KEY_P8_BASE64` | base64 of the `.p8` file |

### What happens in CI

`import` builds a temporary keychain from the `.p12`; `sign` runs
`codesign --options runtime --timestamp` on every bundle (the standalone apps
get `scripts/ci/mac-app.entitlements`, which allows audio input under the
hardened runtime); `pkg` / `dmg` sign the installer, submit it to Apple with
`notarytool --wait`, fail the build if Apple rejects it, and staple the ticket.
The keychain is deleted at the end of the job.

Check a downloaded build on a Mac:

```bash
spctl --assess --type install -vv "RoneStucker_Installer.pkg"
xcrun stapler validate "RoneStucker_Installer.pkg"
```

---

## Windows — Authenticode

Since 2023 code-signing keys must live in certified hardware, so a certificate
cannot simply be exported and used on a GitHub runner: use a cloud signing
service. Three backends are wired in; the script picks whichever secrets exist. Liran is an
individual (no company), so **Option B** is the one to set up.

### Option A — Azure Artifact Signing (formerly Trusted Signing)

Cheapest and cleanest (about $10 / month, unlimited signatures), **but the
identity validation for Israel is available to organizations only** — a
registered company (חברה בע"מ) with a public registry record. Individuals are
accepted only from the US and Canada.

1. Azure subscription (pay-as-you-go; free/trial subscriptions are refused).
2. Create an **Artifact Signing account** (Basic), then **Identity validation →
   Organization** (legal name, registration number, email). Wait for *Completed*.
3. Create a **Certificate profile** of type *Public Trust*.
4. Create a service principal for CI: Microsoft Entra → App registrations →
   New; add a client secret; on the Artifact Signing account give it the role
   **Artifact Signing Certificate Profile Signer**.

| Secret | Value |
|---|---|
| `AZURE_TENANT_ID` | Entra tenant ID |
| `AZURE_CLIENT_ID` | app registration (client) ID |
| `AZURE_CLIENT_SECRET` | its client secret |
| `ACS_ENDPOINT` | the account's endpoint, e.g. `https://weu.codesigning.azure.net` |
| `ACS_ACCOUNT` | the Artifact Signing account name |
| `ACS_PROFILE` | the certificate profile name |

### Option B — SSL.com IV certificate with eSigner  ← **the RONE path (individual, no company)**

An *Individual Validation* (IV) certificate is issued to a person; SSL.com
verifies your identity with a government ID. The private key lives in their
eSigner cloud, which is what the CI uses. Nothing to install, no USB token.

**Cost:** IV Code Signing $129 / year (1 year; $109.65 / year on a 3-year
term) + eSigner *Tier 1* $15 / month (240 signatures a month, roll over; the
first 30 days are free). A full RONE release signs about 30 files, so Tier 1
covers more than six full releases a month.

**Buy (about 10 minutes):**

1. <https://www.ssl.com/products/software-integrity/code-signing/iv/> →
   choose the term → at checkout pick **eSigner cloud signing** as the
   delivery (not the YubiKey; that is the $379 token and cannot be used from
   GitHub's runners).
2. Fill in your legal name exactly as on your ID; the certificate will say
   `CN=Liran Rone Kalifa` and that is the publisher name Windows shows.
3. Identity validation: government-issued ID (passport or ID card), sometimes
   a short video/selfie check. Standard validation takes 3–5 business days.
4. When the certificate is issued, in the SSL.com dashboard open the order →
   **eSigner** → *Enroll*, choose a PIN, and when the **QR code** is shown
   press *show secret code* and copy the text value — that is
   `ESIGNER_TOTP_SECRET` (the CI needs the text, not the QR image). If you
   skipped it, the certificate details page offers to reset it.
5. The **credential ID** is shown on the certificate details page under
   *SIGNING CREDENTIALS* (with a single certificate CodeSignTool would also
   find it by itself: `CodeSignTool get_credential_ids -username=… -password=…`).
6. Subscribe to eSigner Tier 1 (the dashboard asks for it after the free
   month; signing stops when the plan lapses).

**Secrets** (`GitHub → rone-plugins → Settings → Secrets and variables → Actions`):

| Secret | Value |
|---|---|
| `ESIGNER_USERNAME` | your SSL.com account e-mail |
| `ESIGNER_PASSWORD` | your SSL.com account password |
| `ESIGNER_CREDENTIAL_ID` | the credential ID from step 5 |
| `ESIGNER_TOTP_SECRET` | the secret code from step 4 |

The CI downloads SSL.com's CodeSignTool on each run and signs with it; SSL.com
scans every file for malware before signing and refuses flagged binaries.

**Rehearsal without a certificate:** SSL.com publishes a sandbox account.
Setting the four secrets to their demo values plus `ESIGNER_SANDBOX=1` runs
the whole pipeline against the sandbox (signatures chain to a test root, so
never ship those). The demo values are in
<https://www.ssl.com/guide/esigner-demo-credentials-and-certificates/>.

### Option C — a `.pfx` you hold

For a certificate you already own as a file (older certificates, or a
self-signed one for testing the pipeline):

| Secret | Value |
|---|---|
| `WIN_SIGN_PFX_BASE64` | `[Convert]::ToBase64String([IO.File]::ReadAllBytes("cert.pfx"))` |
| `WIN_SIGN_PFX_PASSWORD` | its password |

### What happens in CI

*Sign binaries* signs every PE file in the build output (standalone `.exe`,
the VST3 DLL, the Center). Then Inno Setup is compiled with `/DSIGN` and the
`SignTool=rone` directive in each `.iss`, so it calls the same script for the
uninstaller and for `Setup.exe`. The job fails if the installer comes out
unsigned. Check a downloaded build:

```powershell
Get-AuthenticodeSignature "RoneStucker_Installer.exe" | Format-List Status, SignerCertificate
```

**SmartScreen:** a freshly issued OV certificate (or Artifact Signing
identity) still triggers "Windows protected your PC" for the first weeks;
reputation accumulates with downloads and the prompt disappears. An EV
certificate skips that period but needs a hardware token or a pricier cloud
tier. Submitting a signed installer at
<https://www.microsoft.com/wdsi/filesubmission> speeds the process up.

---

## Turning it on

1. Add the secrets for one platform (or both).
2. Push any change to a plugin, or tag a release, and open the job log:
   *Sign binaries* / *Import signing identity* print which backend was used.
3. Download the installer from the release and verify it with the commands
   above.

If a signing step fails the release is not published — better an absent build
than an unsigned one presented as signed.
