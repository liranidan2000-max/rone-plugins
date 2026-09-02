# ============================================================================
# RONE ALL ACCESS for free - beta testers, friends, reviewers, refunds-by-hand.
#
#   .\scripts\grant-access.ps1 -Email someone@example.com               # forever
#   .\scripts\grant-access.ps1 -Email someone@example.com -Days 90      # 90 days
#   .\scripts\grant-access.ps1 -Email someone@example.com -Devices 3
#   .\scripts\grant-access.ps1 -Email someone@example.com -Revoke
#   .\scripts\grant-access.ps1 -Email someone@example.com -Show         # read only
#
# What it does (directly in the D1 database "rone-accounts", the same rows a
# Paddle purchase writes):
#   * creates the user row if the address is new (source "comp"),
#   * upserts the "bundle" entitlement: status active, expires_at NULL (=forever)
#     or now + Days, device_limit as given.
#
# What the person then does:
#   1. If they already signed up on roneaudio.com: nothing - sign in inside the
#      Plugins Center (Account panel) and every plugin unlocks.
#   2. If they never signed up: go to roneaudio.com/account/signup with that
#      exact e-mail (it "claims" the pre-created account and sets the
#      password), then sign in inside the Center. "Forgot password" works too.
#
# Needs: the portable Node in D:\RONE PLUGINS\tools and wrangler logged in
# (npx wrangler whoami) from the website folder.
# ============================================================================
param(
    [Parameter(Mandatory = $true)] [string]$Email,
    [int]$Days = 0,
    [int]$Devices = 2,
    [string]$Name = "",
    [switch]$Revoke,
    [switch]$Show
)

$site = "D:\RONE PLUGINS\WEBSITE RONE AUDIO"
$node = "D:\RONE PLUGINS\tools\node-v24.20.0-win-x64"
if (-not (Test-Path (Join-Path $site "wrangler.toml"))) { Write-Error "website folder not found: $site"; exit 1 }
$env:PATH = "$node;" + $env:PATH

$email = $Email.Trim().ToLowerInvariant()
if ($email -notmatch '^[^@\s]+@[^@\s]+\.[^@\s]+$') { Write-Error "not an e-mail address: $Email"; exit 1 }
$q = $email -replace "'", "''"
$now = [int64]([DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds())

function Invoke-D1([string]$sql) {
    Push-Location $site
    try {
        & npx wrangler d1 execute rone-accounts --remote --command $sql
        if ($LASTEXITCODE -ne 0) { throw "wrangler d1 execute failed" }
    } finally { Pop-Location }
}

if ($Show) {
    Invoke-D1 ("SELECT u.email, u.name, CASE WHEN u.password_hash IS NULL THEN 'no' ELSE 'yes' END AS has_password, " +
               "e.product, e.status, e.source, e.device_limit, " +
               "CASE WHEN e.expires_at IS NULL THEN 'never' ELSE datetime(e.expires_at/1000,'unixepoch') END AS expires, " +
               "(SELECT count(*) FROM devices d WHERE d.user_id = u.id AND d.revoked_at IS NULL) AS devices " +
               "FROM users u LEFT JOIN entitlements e ON e.user_id = u.id WHERE u.email = '$q';")
    exit 0
}

if ($Revoke) {
    Invoke-D1 ("UPDATE entitlements SET status = 'disabled', updated_at = $now " +
               "WHERE product = 'bundle' AND user_id = (SELECT id FROM users WHERE email = '$q');")
    Write-Host "ALL ACCESS disabled for $email (their Center locks at its next validation)." -ForegroundColor Yellow
    exit 0
}

$userId = [guid]::NewGuid().ToString()
$entId  = [guid]::NewGuid().ToString()
$nameSql = if ($Name) { "'" + ($Name -replace "'", "''") + "'" } else { "NULL" }
$expires = if ($Days -gt 0) { [string]($now + [int64]$Days * 86400000) } else { "NULL" }

$sql = @"
INSERT INTO users (id, email, name, created_at) VALUES ('$userId', '$q', $nameSql, $now)
  ON CONFLICT(email) DO UPDATE SET name = COALESCE(users.name, excluded.name);
INSERT INTO entitlements (id, user_id, product, status, source, device_limit, expires_at, renews_at, created_at, updated_at)
  VALUES ('$entId', (SELECT id FROM users WHERE email = '$q'), 'bundle', 'active', 'comp', $Devices, $expires, NULL, $now, $now)
  ON CONFLICT(user_id, product) DO UPDATE SET
    status = 'active', source = 'comp', device_limit = excluded.device_limit,
    expires_at = excluded.expires_at, renews_at = NULL, updated_at = excluded.updated_at;
"@

Invoke-D1 $sql
$until = if ($Days -gt 0) { "for $Days days (until $([DateTimeOffset]::FromUnixTimeMilliseconds($now + [int64]$Days * 86400000).ToString('yyyy-MM-dd')))" } else { "forever" }
Write-Host ""
Write-Host "ALL ACCESS granted to $email $until, $Devices device(s)." -ForegroundColor Green
Write-Host "Tell them: sign in inside the RONE Plugins Center with this e-mail."
Write-Host "No account yet? roneaudio.com/account/signup with this e-mail sets the password and claims it."
