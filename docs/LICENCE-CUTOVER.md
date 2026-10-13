# Licence cut-over: measured, staged rollout

Audit items R004, R005, R014, R042, R047 and R049 (quality audit 2026-10-12,
reconciled 2026-10-13). This is the plan for moving every RONE product from
"a plain BundleLicense.xml counts" to "only a server-signed, machine-bound
entitlement counts", without locking out one paying customer.

The rule throughout: **measure first, then switch one thing at a time, each
with a rollback.** Nothing in this document has been switched. The Shared
branch `audit/w8-shared` adds measurement only; no access becomes stricter.

---

## 1. Where things stand (2026-10-13)

| Switch | Where | Today |
|---|---|---|
| `license_mode` | versions.json | `"enforced"` (no free mode) |
| `signed_licences` | versions.json, top level | **absent**, so nothing requires signatures |
| `kSignedRequiredFromMs` | Shared/RemoteLicenseGate.h, compiled in | 2027-03-01 00:00 UTC, in every build from CI run #250 |
| `SIGNED_LICENCES_REQUIRED` | Worker var | not set; the Worker does not use it yet (Stage 1b) |
| `ENTITLEMENT_MACHINE_BIND` | Worker var | does not exist yet (Stage 5) |

**How each side behaves:**

- **Centers that write signed files:** 2.0.0.250 to 2.1.2.264. They write
  `ent`/`sig` whenever the Worker signs, and they send `machineId` on every
  `/api/v1/app/refresh`.
- **Centers that write plain files only:** 1.3.x to 1.6.2.249. They send an
  empty refresh body (the Worker stamps `devices.legacy_center_at`). Their
  2-argument `writeMode` drops `signed=` from LicenseMode.xml on every refresh.
  The Mac 1.3.x Center cannot update itself.
- **Plugins that read signed files:** CI run #250 and later, plus Stutter
  1.5.7.268 or later and the Analyzer 1.3.11.268 or later. A valid `ent`/`sig`
  is the only truth. Without one, the plain attributes count until
  `signedLicencesRequired()` (the flag, or the 2027-03-01 date).
- **Older plugins:** trust plain files forever, ignore the flag, and drop
  `signed=` on their own 6 h refresh.

**What the flip would hit:** a machine is at risk when it has a
signed-reading plugin and a BundleLicense.xml without a valid signature.
In practice that means:

1. a 1.x Center (it keeps installing current plugins, so it does have
   signed-reading builds), which also makes the machine flap between locked
   and unlocked as the Center and the plugins rewrite LicenseMode.xml;
2. a Lemon Squeezy serial with no account sign-in;
3. a signing outage on the Worker (the Center gets an unsigned 200 today).

**What `audit/w8-shared` adds (next Shared rebuild, all products). All of it
reports, none of it locks:**

| Code queued (crash/report queue, at most once per 30 days per computer) | Meaning | Feeds |
|---|---|---|
| `LICENCE_MACHINE_MISMATCH` (details: `host=<DAW exe>; local ids=<n>`) | A signed licence verifies, but its `m` is neither of this computer's machine ids, as the plugin computes them (sha256 of `rone-device:` + JUCE device id, or of machine-id.txt). The licence still counts. | Stage 5 (R014): how often a genuine owner's DAW disagrees with his Center |
| `LICENCE_MODE_UNWRITABLE` (details: `host=<DAW exe>`) | LicenseMode.xml says open and the plugin cannot rewrite it, so the file is frozen. Nothing changes for that machine. | Stage 6 (R005) |

The reports reach `rone-crash-reports` through the Center's uploader (2.x via
`/api/v1/crash`). A 1.x Center filed with its own compiled token, which
cannot be counted on, so the 1.x group is measured server-side (Stage 0).
`Shared/tests/licence_sim` runs the whole chain offline: plain, signed,
tampered, serial and slow-clock cases, the grace windows, machine-id parity
with juce::SHA256, and a yes/no check that this computer's real signed
licence names this computer.

---

## 2. Stage 0: measure, read-only (no gate change)

The owner runs these as aggregate counts in the D1 console. No customer row
leaves D1 and no customer is contacted by this stage. Before running them,
check the actual values with
`SELECT source, status, COUNT(*) FROM entitlements GROUP BY 1, 2`.

**0a. Active licence holders whose latest refresh came from a 1.x Center (R049, the
group the flip would lock):**

Every source counts - paid, gift, free and trial alike - because the flip
locks all of them, and the window is 90 days (the owner grace), not 30: a
customer who opened the Center two months ago is still a customer.

```sql
SELECT d.platform, COUNT(DISTINCT d.user_id) AS users, COUNT(*) AS devices
FROM devices d
WHERE d.revoked_at IS NULL
  AND d.last_seen_at >= (strftime('%s','now') * 1000) - 90 * 86400000
  AND d.legacy_center_at IS NOT NULL
  AND d.legacy_center_at >= d.last_seen_at - 86400000
  AND EXISTS (SELECT 1 FROM entitlements e
              WHERE e.user_id = d.user_id AND e.status = 'active'
                AND (e.expires_at IS NULL OR e.expires_at > strftime('%s','now') * 1000))
GROUP BY d.platform;
```

**0b. Active Lemon Squeezy entitlements, test orders excluded.** The same
pattern on `entitlements` with the LS source value. Expected about 0, because
LS never approved the account.

**0c. Center versions in the field.** This cannot be measured today: the
refresh body carries only `machineId`. It needs Stage 1c (`centerVersion` in
the body, stored best-effort in `devices.center_version`). After that:
`SELECT center_version, COUNT(*) FROM devices WHERE revoked_at IS NULL AND last_seen_at > <30 d ago> GROUP BY 1`.

**0d. Plugin builds in the field.** The server never sees them. The
`LICENCE_*` reports carry product + version. For the flip, 0a already covers
the case that matters (a 1.x Center installs current, signed-reading builds).

**0e. Unsigned answers to signed Centers.** Count refreshes where
`signEntitlement` returned null (Stage 1a adds the counter). This must be 0
before the flip.

**0f. R014 baseline.** From the first Shared rebuild that carries
`audit/w8-shared`, count `LICENCE_MACHINE_MISMATCH` issues per week, grouped
by the `host=` value. Expected: near 0 on Windows. A sandboxed macOS AU
host (Logic's AUHostingService) is the open question this answers.

**0g. Health card.** `health.js licenseMode()` also shows
`signedLicences: v.signed_licences || null`, so the console always says which
way the switch stands.

---

## 3. Stage 1: safety nets (no gate change)

- **1a. site-worker: signing heartbeat.** A cron signs a dummy payload and
  writes `system_state 'signing_tick' {ok, at}`. The ops-probe reads it like
  `cron_tick` and pushes to the iPhone when it is stale or failed.
- **1b. site-worker refresh.js.** When `env.SIGNED_LICENCES_REQUIRED === '1'`,
  the body has a `machineId`, and `signEntitlement` returned null, answer
  `503 signing_unavailable` instead of an unsigned 200. A 2.x Center treats a
  non-401 as offline and keeps its last signed `ent`/`sig`. A 1.x Center (no
  `machineId`) gets today's answer. With the var unset nothing changes.
- **1c. Center, the release after 2.1.3.**
  - While signed licences are required, keep the previous `ent`/`sig` when an
    ok answer arrives unsigned, unless the claim narrowed (pass true to
    false, or `owned` shrank).
  - Show a non-blocking "sign in again to keep your plugins" message when the
    file is licensed but its `ent`/`sig` does not verify.
  - Send `centerVersion` in the refresh body (feeds 0c).
- **1d. scripts/set-license-mode.ps1.** A new `-SignedLicences required|off`
  adds or removes the top-level key. The CI `jq` bump keeps unknown keys.

**Gate to Stage 2:** 1a-1c released, and `signing_tick` green for 14 days in
a row.

---

## 4. Stage 2: rehearsal (Windows VM + a Mac, hosts-overridden manifest)

| Machine | Expected after the flip |
|---|---|
| Center 2.1.x signed in, current plugins | unlocked, no change |
| Center 1.6.2 + a #250+ plugin | locks, and flaps every 6 h (the 0a group) |
| LS serial file, no account | locks |
| Hand-written plain file | locks |
| LicenseMode.xml made read-only before the flip | stays as it was until 2027-03-01 (accepted; R005 observation) |
| Clock 30 min slow, signed file | unlocked (R182) |

---

## 5. Stage 3: the switch (R004)

**Preconditions, all of them:**

- 0a = 0 and 0b = 0, or each remaining account solved one by one by hand
  (support message with the current installer link, never a mass mail).
- 0e = 0 for 14 days.
- `signing_tick` green for 14 days.
- The Stage 1c Center has been released for at least 30 days and runs on at
  least 95 % of the devices seen in the last 30 days (0c).
- Weekday morning (Israel), and the owner is available for 48 h.

**The switch:**

1. `scripts/set-license-mode.ps1 -SignedLicences required`. This adds
   `"signed_licences": "required"` to versions.json in a `[skip ci]` commit.
2. Set the Worker var `SIGNED_LICENCES_REQUIRED=1`.

**Propagation:** Centers on launch, on focus and every 6 h. Plugins on their
own fetch (6 h, or 10 min after a failed try, from `audit/w8-shared` on),
plus up to 5 min of CDN cache. **Watch:** the support inbox and the console
for 7 days.

**Rollback:** remove the key (`-SignedLicences off`, `[skip ci]`) and unset
the var. Online machines relax on their next fetch, within 6 h. An offline
machine keeps the cached flag until it fetches. There is no rollback for
builds from #250 on after 2027-03-01 (the compiled date, R047).

---

## 6. Stage 4: the compiled date (R047)

- The owner confirms 2027-03-01. It cannot move for builds already shipped.
- A later date helps only users who update, and it must be a **new compiled
  constant**. Never an unsigned versions.json or LicenseMode.xml field:
  those are user-editable, and "later" is the bypass direction.
- From 2027-02-01, contact whoever is still in 0a one by one.
- Each plugin's next release: the lock card shows
  `RemoteLicenseGate::getLockMessage()`.
- Optional, owner decision: a signed rescue lever (`license_override
  {open_until, sig}` in versions.json, verified with the compiled public key,
  short expiry).

---

## 7. Stage 5: machine binding (R014; R042 is its server half)

This stage runs only after Stage 3 has been stable for 30 days.

1. **Observe.** Already in `audit/w8-shared`. Run for 2-4 weeks after that
   Shared rebuild is in the field (0f). **Gate:** every
   `LICENCE_MACHINE_MISMATCH` host is explained, and the owner's own machines
   (Windows FL, Ableton and Cubase; Mac Logic, Ableton and FL) show no
   report. On those machines, licence_sim's real-file check says yes.
2. **Enforce code.** A Shared rebuild:
   - `RoneEntitlement::read` parses an optional int `mb` into
     `Payload::bind`, still `v == 1`;
   - in `parseLicenseFile`, `bind && isOtherMachine(...)` returns an empty
     state (the file claims nothing);
   - an unknown local id never locks.

   Without `mb` in the payload nothing changes, so this ships inert.
3. **Server switch.** site `signing.js` adds `mb: 1` only when
   `env.ENTITLEMENT_MACHINE_BIND` is `'all'` or lists the user id.
   - **Canary:** the owner's account for 14 days.
   - **Then:** `'all'`.
4. **Rollback:** unset the var. Each Center rewrites `ent` without `mb` on its
   next refresh (launch, focus or 24 h). Older builds ignore `mb`.

**Do not:**

- add `exp` (iat plus the grace already bounds validity);
- shorten the 90-day owner grace;
- put any switch in LicenseMode.xml;
- make refresh.js require `machineId` (R042). 1.x Centers send `{}`, and on
  a 401 they delete Account.xml and BundleLicense.xml: a permanent sign-out
  loop.

**Mac token:** move it to the Keychain only once Apple code signing exists
(owner decision).

---

## 8. Stage 6: open mode (R005)

- The observation (`LICENCE_MODE_UNWRITABLE`) is in `audit/w8-shared`.
- **Enforce step** (later Shared rebuild, after the owner decides whether
  new builds honour `license_mode: "open"` at all): in a process whose
  attempt write failed, `isOpenMode()` returns false. Alternatively, drop the
  open branch, or replace it with the signed lever from Stage 4.
- **Risk:** none for payers. A paying customer's BundleLicense.xml does not
  depend on open mode.

---

## 9. Calendar

| When | What | Gate |
|---|---|---|
| Next Shared rebuild | `audit/w8-shared` in all products (observe only) | builds green, licence_sim pass |
| +0-2 weeks | Stage 0 counts, Stage 1a/1b (Worker), 1d (script) | owner approves the D1 aggregates |
| Center after 2.1.3 | Stage 1c | release QA |
| +30 days after 1c | Stage 2 rehearsal | 1c on ≥ 95 % of devices |
| First weekday after all Stage 3 preconditions | **Stage 3 flip** | 0a = 0b = 0e = 0, signing_tick 14 d green |
| Flip + 30 days | Stage 5 enforce code (inert) in a routine Shared rebuild | 0f explained |
| + 14 days canary | `ENTITLEMENT_MACHINE_BIND` = owner, then `all` | no canary report |
| 2027-02-01 | one-by-one contact of whoever remains in 0a | - |
| 2027-03-01 | compiled cut-over (already in every build since #250) | owner confirmed |
