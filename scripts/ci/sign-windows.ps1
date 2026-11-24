<#
.SYNOPSIS
    Authenticode-signs RONE build outputs (plugins, standalones, the Center,
    installers). Used by .github/workflows/main.yml and by Inno Setup (for the
    uninstaller + Setup.exe); works locally too.

.DESCRIPTION
    The signing backend is chosen by which environment variables are set
    (GitHub secrets in CI). First complete set wins:

      Azure Artifact Signing (Trusted Signing) - organisations, ~$10/month
        AZURE_TENANT_ID, AZURE_CLIENT_ID, AZURE_CLIENT_SECRET   (service principal)
        ACS_ENDPOINT   e.g. https://weu.codesigning.azure.net
        ACS_ACCOUNT    the Artifact Signing account name
        ACS_PROFILE    the certificate profile name

      Azure Key Vault - the RONE path since 24.11.2026: SSL.com IV certificate, key in our vault's HSM
        AZURE_TENANT_ID, AZURE_CLIENT_ID and a GitHub job with `id-token: write`: the job proves who it
        is to Azure by itself (OIDC federation), there is no client secret anywhere. No ACS_* set.
        AKV_VAULT, AKV_CERT   optional, default to the RONE vault and certificate below
        AKV_USE_AZ_CLI=1      on Liran's PC only: sign with the `az login` session, no secrets needed

      SSL.com eSigner - individuals, IV/OV certificate in their cloud (not used: ours is in Key Vault)
        ESIGNER_USERNAME, ESIGNER_PASSWORD, ESIGNER_CREDENTIAL_ID, ESIGNER_TOTP_SECRET
        ESIGNER_SANDBOX=1 (optional) rehearses against SSL.com's sandbox with their demo account

      PFX file - any certificate you hold as a .pfx (testing, self-hosted)
        WIN_SIGN_PFX_BASE64, WIN_SIGN_PFX_PASSWORD

    With none of them set the script prints a notice and exits 0, so the
    pipeline keeps producing unsigned builds until the secrets exist.
    See docs/CODE-SIGNING.md.

.PARAMETER Status
    Print the backend that would be used (acs | akv | esigner | pfx | none) and exit.
.PARAMETER Require
    Fail instead of skipping when no backend is configured (used once signing
    is expected, e.g. by Inno Setup).
.PARAMETER Paths
    Files or folders. Folders are searched recursively for *.exe, *.dll and
    *.vst3 PE files (a .vst3 bundle's binary is a DLL named *.vst3).

.EXAMPLE
    scripts\ci\sign-windows.ps1 RoneStucker\build-ci\RoneStucker_artefacts\Release
    scripts\ci\sign-windows.ps1 -Require build-output\RoneStucker_Installer.exe
#>
[CmdletBinding()]
param(
    [switch] $Status,
    [switch] $Require,
    [Parameter(ValueFromRemainingArguments = $true)] [string[]] $Paths
)

$ErrorActionPreference = 'Stop'

# Names, not secrets: the RONE vault and the certificate in it (docs/CODE-SIGNING.md).
$akvVault = $(if ($env:AKV_VAULT) { $env:AKV_VAULT } else { 'rone-codesign-kv' })
$akvCert  = $(if ($env:AKV_CERT)  { $env:AKV_CERT }  else { 'rone-codesign-2026' })

function Has([string[]] $names) {
    foreach ($n in $names) { if ([string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable($n))) { return $false } }
    return $true
}

function Detect-Backend {
    if (Has @('AZURE_TENANT_ID', 'AZURE_CLIENT_ID', 'AZURE_CLIENT_SECRET', 'ACS_ENDPOINT', 'ACS_ACCOUNT', 'ACS_PROFILE')) { return 'acs' }
    if ((Has @('AZURE_TENANT_ID', 'AZURE_CLIENT_ID', 'ACTIONS_ID_TOKEN_REQUEST_URL', 'ACTIONS_ID_TOKEN_REQUEST_TOKEN')) -and -not (Has @('ACS_ACCOUNT'))) { return 'akv' }
    if ($env:AKV_USE_AZ_CLI -eq '1') { return 'akv' }
    if (Has @('ESIGNER_USERNAME', 'ESIGNER_PASSWORD', 'ESIGNER_CREDENTIAL_ID', 'ESIGNER_TOTP_SECRET')) { return 'esigner' }
    if (Has @('WIN_SIGN_PFX_BASE64', 'WIN_SIGN_PFX_PASSWORD')) { return 'pfx' }
    foreach ($partial in @(@('AZURE_TENANT_ID', 'ACS_ACCOUNT'), @('ESIGNER_USERNAME'), @('WIN_SIGN_PFX_BASE64'))) {
        foreach ($n in $partial) {
            if (-not [string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable($n))) {
                Write-Warning "$n is set but the rest of that backend's variables are missing - treating as unsigned. See docs/CODE-SIGNING.md."
            }
        }
    }
    return 'none'
}

$backend = Detect-Backend
if ($Status) { Write-Output $backend; exit 0 }

if ($backend -eq 'none') {
    if ($Require) { Write-Error 'Code signing was required but no signing secrets are configured (docs/CODE-SIGNING.md).'; exit 1 }
    Write-Host '::notice::No Windows code-signing secrets configured - this build is UNSIGNED.'
    exit 0
}
if (-not $Paths -or $Paths.Count -eq 0) { Write-Error 'Nothing to sign: pass files or folders.'; exit 1 }

# ---- work folder (cached tools) -------------------------------------------------
$work = Join-Path ($(if ($env:RUNNER_TEMP) { $env:RUNNER_TEMP } else { $env:TEMP })) 'rone-sign'
New-Item -ItemType Directory -Force -Path $work | Out-Null

function Find-SignTool {
    $roots = @("${env:ProgramFiles(x86)}\Windows Kits\10\bin", "$env:ProgramFiles\Windows Kits\10\bin")
    $found = foreach ($r in $roots) {
        if (Test-Path $r) { Get-ChildItem $r -Recurse -Filter signtool.exe -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match '\\x64\\' } }
    }
    $best = $found | Sort-Object { [version]($_.Directory.Parent.Name -replace '[^\d.]', '') } -Descending | Select-Object -First 1
    if (-not $best) { throw 'signtool.exe not found - install a Windows 10/11 SDK.' }
    return $best.FullName
}

# ---- collect the files ------------------------------------------------------------
$files = New-Object System.Collections.Generic.List[string]
foreach ($p in $Paths) {
    if (Test-Path $p -PathType Container) {
        Get-ChildItem $p -Recurse -File -Include *.exe, *.dll, *.vst3 | ForEach-Object { $files.Add($_.FullName) }
    } elseif (Test-Path $p -PathType Leaf) {
        $files.Add((Resolve-Path $p).Path)
    } else {
        Write-Warning "Not found, skipped: $p"
    }
}
# only PE images (a .vst3 *folder* is skipped above; its inner DLL named .vst3 is what we want)
$pe = foreach ($f in $files) {
    try {
        $fs = [IO.File]::OpenRead($f); $b = New-Object byte[] 2; [void]$fs.Read($b, 0, 2); $fs.Dispose()
        if ($b[0] -eq 0x4D -and $b[1] -eq 0x5A) { $f }
    } catch { }
}
$pe = @($pe | Select-Object -Unique)
if ($pe.Count -eq 0) { Write-Warning 'No PE files found to sign.'; exit 0 }

Write-Host "Signing $($pe.Count) file(s) with backend '$backend':"
$pe | ForEach-Object { Write-Host "  $_" }

# ---- backends ---------------------------------------------------------------------
$signtool = $null
if ($backend -ne 'esigner') { $signtool = Find-SignTool; Write-Host "signtool: $signtool" }

function Sign-Acs([string[]] $targets) {
    # Microsoft.Trusted.Signing.Client NuGet ships the dlib signtool uses to sign in Azure.
    $dlibDir = Join-Path $work 'acs'
    $dlib = Get-ChildItem $dlibDir -Recurse -Filter Azure.CodeSigning.Dlib.dll -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\x64\\' } | Select-Object -First 1
    if (-not $dlib) {
        New-Item -ItemType Directory -Force -Path $dlibDir | Out-Null
        $nupkg = Join-Path $dlibDir 'client.zip'
        Write-Host 'Downloading Microsoft.Trusted.Signing.Client...'
        Invoke-WebRequest -Uri 'https://www.nuget.org/api/v2/package/Microsoft.Trusted.Signing.Client' -OutFile $nupkg -MaximumRedirection 5
        Expand-Archive $nupkg -DestinationPath $dlibDir -Force
        $dlib = Get-ChildItem $dlibDir -Recurse -Filter Azure.CodeSigning.Dlib.dll | Where-Object { $_.FullName -match '\\x64\\' } | Select-Object -First 1
        if (-not $dlib) { throw 'Azure.CodeSigning.Dlib.dll not found in the NuGet package.' }
    }
    $meta = Join-Path $work 'acs-metadata.json'
    @{
        Endpoint               = $env:ACS_ENDPOINT.TrimEnd('/') + '/'
        CodeSigningAccountName = $env:ACS_ACCOUNT
        CertificateProfileName = $env:ACS_PROFILE
        # Outside Azure only the service-principal (environment) credential can work.
        ExcludeCredentials     = @('ManagedIdentityCredential', 'WorkloadIdentityCredential', 'SharedTokenCacheCredential',
                                   'VisualStudioCredential', 'VisualStudioCodeCredential', 'AzureCliCredential',
                                   'AzurePowerShellCredential', 'AzureDeveloperCliCredential', 'InteractiveBrowserCredential')
    } | ConvertTo-Json | Set-Content -Path $meta -Encoding ascii

    foreach ($t in $targets) {
        & $signtool sign /v /fd SHA256 /tr http://timestamp.acs.microsoft.com /td SHA256 /dlib $dlib.FullName /dmdf $meta $t
        if ($LASTEXITCODE -ne 0) { throw "signtool failed for $t" }
    }
}

function Invoke-Akv([string] $method, [string] $uri, $headers, $body, [string] $contentType) {
    # Azure answers 429 / 5xx now and then: a whole release must not fail on one of those.
    # 400 / 401 / 403 / 404 are ours (wrong secret, missing role) - no point in repeating them.
    for ($try = 1; ; $try++) {
        try {
            $a = @{ Method = $method; Uri = $uri; TimeoutSec = 60; UseBasicParsing = $true }
            if ($headers) { $a.Headers = $headers }
            if ($null -ne $body) { $a.Body = $body; $a.ContentType = $contentType }
            return Invoke-RestMethod @a
        } catch {
            $code = 0
            if ($_.Exception.Response) { $code = [int]$_.Exception.Response.StatusCode }
            if ($try -ge 5 -or ($code -ge 400 -and $code -lt 500 -and $code -ne 429 -and $code -ne 408)) {
                throw "Azure call failed (HTTP $code) on $(([uri]$uri).Host)$(([uri]$uri).AbsolutePath): $($_.Exception.Message)"
            }
            Start-Sleep -Seconds (3 * $try)
        }
    }
}

function Get-AkvToken {
    if ($env:AKV_USE_AZ_CLI -eq '1') {
        $t = (& az account get-access-token --resource https://vault.azure.net --query accessToken -o tsv)
        if ($LASTEXITCODE -ne 0 -or -not $t) { throw 'az has no session - run: az login' }
        return "$t".Trim()
    }
    # GitHub signs a short-lived statement "I am this repo, branch main"; Entra trusts exactly that
    # subject (the federated credential on rone-ci-codesign) and gives back a vault token.
    $id = Invoke-Akv 'Get' "$env:ACTIONS_ID_TOKEN_REQUEST_URL&audience=api%3A%2F%2FAzureADTokenExchange" @{
        Authorization = "Bearer $env:ACTIONS_ID_TOKEN_REQUEST_TOKEN" } $null $null
    $r = Invoke-Akv 'Post' "https://login.microsoftonline.com/$env:AZURE_TENANT_ID/oauth2/v2.0/token" $null @{
        grant_type = 'client_credentials'; client_id = $env:AZURE_CLIENT_ID
        client_assertion_type = 'urn:ietf:params:oauth:client-assertion-type:jwt-bearer'
        client_assertion = $id.value; scope = 'https://vault.azure.net/.default'
    } 'application/x-www-form-urlencoded'
    return $r.access_token
}

function Sign-Akv([string[]] $targets) {
    # signtool's own three-step "digest signing": it writes the digest (/dg), Key Vault signs that
    # digest inside its HSM, signtool puts the result back (/di). The key never leaves the vault
    # and no third-party signing tool ever holds our token.
    $auth = @{ Authorization = "Bearer $(Get-AkvToken)" }
    $cert = Invoke-Akv 'Get' "https://$akvVault.vault.azure.net/certificates/${akvCert}?api-version=7.4" $auth $null $null
    $run = Join-Path $work ('akv-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Force -Path $run | Out-Null
    try {
        $cer = Join-Path $run 'signer.cer'
        [IO.File]::WriteAllBytes($cer, [Convert]::FromBase64String($cert.cer))
        $x = New-Object Security.Cryptography.X509Certificates.X509Certificate2 (,[IO.File]::ReadAllBytes($cer))
        if ($x.NotAfter -lt (Get-Date)) { throw "The code-signing certificate expired on $($x.NotAfter) - renew it (docs/CODE-SIGNING.md)." }
        if ($x.NotAfter -lt (Get-Date).AddDays(45)) { Write-Host "::warning::The code-signing certificate expires on $($x.NotAfter) - renew it now." }
        Write-Host "Key Vault $akvVault, certificate $akvCert [$($x.Subject)], valid to $($x.NotAfter.ToString('yyyy-MM-dd'))"
        $chain = Join-Path $PSScriptRoot 'win-cert\sslcom-codesigning-intermediate-rsa-r1.crt'
        if (-not (Test-Path $chain)) { throw "Missing $chain" }

        $n = 0
        foreach ($t in $targets) {
            # a vendor's own valid signature (a Microsoft runtime DLL) stays theirs
            $had = Get-AuthenticodeSignature -FilePath $t
            if ($had.Status -eq 'Valid' -and $had.SignerCertificate.Subject -ne $x.Subject) {
                Write-Host "  already signed by [$($had.SignerCertificate.Subject)], left as is: $(Split-Path $t -Leaf)"
                continue
            }
            $n++
            $dir = Join-Path $run "$n"
            New-Item -ItemType Directory -Force -Path $dir | Out-Null
            & $signtool sign /q /dg $dir /fd SHA256 /f $cer /ac $chain $t
            if ($LASTEXITCODE -ne 0) { throw "signtool digest failed for $t" }
            $dig = @(Get-ChildItem -LiteralPath $dir -Filter *.dig)
            if ($dig.Count -ne 1) { throw "signtool wrote $($dig.Count) digests for $t" }
            $b64 = ([IO.File]::ReadAllText($dig[0].FullName)).Trim()
            if ([Convert]::FromBase64String($b64).Length -ne 32) { throw "Unexpected digest length for $t" }
            $json = @{ alg = 'RS256'; value = $b64.TrimEnd('=').Replace('+', '-').Replace('/', '_') } | ConvertTo-Json -Compress
            # kid carries the key VERSION, so the signature always belongs to the certificate above
            $sig = Invoke-Akv 'Post' "$($cert.kid)/sign?api-version=7.4" $auth $json 'application/json'
            $v = $sig.value.Replace('-', '+').Replace('_', '/')
            while ($v.Length % 4) { $v += '=' }
            [IO.File]::WriteAllText($dig[0].FullName + '.signed', $v)
            & $signtool sign /q /di $dir $t
            if ($LASTEXITCODE -ne 0) { throw "signtool ingest failed for $t" }

            # Without a timestamp the signature dies with the certificate (a year): two servers, then fail.
            $stamped = $false
            foreach ($try in 1..3) {
                foreach ($tsa in 'http://timestamp.digicert.com', 'http://ts.ssl.com') {
                    & $signtool timestamp /q /tr $tsa /td SHA256 $t
                    if ($LASTEXITCODE -eq 0) { $stamped = $true; break }
                }
                if ($stamped) { break }
                Start-Sleep -Seconds (5 * $try)
            }
            if (-not $stamped) { throw "No timestamp server answered for $t" }
        }
    } finally {
        Remove-Item -LiteralPath $run -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Sign-Pfx([string[]] $targets) {
    $pfx = Join-Path $work 'cert.pfx'
    [IO.File]::WriteAllBytes($pfx, [Convert]::FromBase64String($env:WIN_SIGN_PFX_BASE64))
    try {
        foreach ($t in $targets) {
            & $signtool sign /v /f $pfx /p $env:WIN_SIGN_PFX_PASSWORD /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 $t
            if ($LASTEXITCODE -ne 0) { throw "signtool failed for $t" }
        }
    } finally {
        Remove-Item $pfx -Force -ErrorAction SilentlyContinue
    }
}

function Sign-ESigner([string[]] $targets) {
    # SSL.com CodeSignTool (Java). Latest Windows build from GitHub releases.
    $toolDir = Join-Path $work 'esigner'
    $bat = Get-ChildItem $toolDir -Recurse -Filter CodeSignTool.bat -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $bat) {
        New-Item -ItemType Directory -Force -Path $toolDir | Out-Null
        $headers = @{ 'User-Agent' = 'rone-plugins-ci' }
        if ($env:GITHUB_TOKEN) { $headers['Authorization'] = "Bearer $env:GITHUB_TOKEN" }
        $rel = Invoke-RestMethod -Uri 'https://api.github.com/repos/SSLcom/CodeSignTool/releases/latest' -Headers $headers
        $asset = $rel.assets | Where-Object { $_.name -match 'windows' -and $_.name -like '*.zip' } | Select-Object -First 1
        if (-not $asset) { throw 'CodeSignTool Windows release asset not found.' }
        $zip = Join-Path $toolDir 'CodeSignTool.zip'
        Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zip
        Expand-Archive $zip -DestinationPath $toolDir -Force
        $bat = Get-ChildItem $toolDir -Recurse -Filter CodeSignTool.bat | Select-Object -First 1
        if (-not $bat) { throw 'CodeSignTool.bat not found after download.' }
    }
    # ESIGNER_SANDBOX=1 points the tool at SSL.com's sandbox (demo credentials, test root):
    # lets the whole pipeline be rehearsed before a real certificate exists.
    $conf = Join-Path $bat.Directory.FullName 'conf/code_sign_tool.properties'
    if ($env:ESIGNER_SANDBOX -eq '1') {
        Write-Host '::warning::eSigner SANDBOX signing - the signature chains to a TEST root, not for release.'
        @('CLIENT_ID=qOUeZCCzSqgA93acB3LYq6lBNjgZdiOxQc-KayC3UMw',
          'OAUTH2_ENDPOINT=https://oauth-sandbox.ssl.com/oauth2/token',
          'CSC_API_ENDPOINT=https://cs-try.ssl.com',
          'TSA_URL=http://ts.ssl.com') | Set-Content -Path $conf -Encoding ascii
    } elseif ((Test-Path $conf) -and (Select-String -Path $conf -Pattern 'sandbox|cs-try' -Quiet)) {
        throw "CodeSignTool config at $conf still points at the sandbox - delete the cached tool folder."
    }
    # CodeSignTool.bat resolves its bundled JDK and jar relative to the current
    # directory (unquoted, so spaces would break it): call java on the jar directly
    # from inside the tool folder, where conf/code_sign_tool.properties is read.
    $toolHome = $bat.Directory.FullName
    $jar  = Get-ChildItem (Join-Path $toolHome 'jar') -Filter 'code_sign_tool*.jar' | Select-Object -First 1
    $java = Get-ChildItem $toolHome -Directory -Filter 'jdk-*' | ForEach-Object { Join-Path $_.FullName 'bin/java.exe' } | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $java) { $java = 'java' }   # CI runners ship a JDK on PATH
    if (-not $jar)  { throw "code_sign_tool jar not found under $toolHome" }
    Push-Location $toolHome
    try {
        foreach ($t in $targets) {
            # WINDOWS-ROOT: trust what Windows trusts - the bundled JDK 11 truststore is from 2019
            # and rejects SSL.com's current chain ("PKIX path building failed").
            & $java '-Djavax.net.ssl.trustStoreType=WINDOWS-ROOT' -jar $jar.FullName sign "-username=$env:ESIGNER_USERNAME" "-password=$env:ESIGNER_PASSWORD" `
                "-credential_id=$env:ESIGNER_CREDENTIAL_ID" "-totp_secret=$env:ESIGNER_TOTP_SECRET" `
                "-input_file_path=$t" -override
            if ($LASTEXITCODE -ne 0) { throw "CodeSignTool failed for $t" }
        }
    } finally {
        Pop-Location
    }
}

switch ($backend) {
    'acs'     { Sign-Acs $pe }
    'akv'     { Sign-Akv $pe }
    'esigner' { Sign-ESigner $pe }
    'pfx'     { Sign-Pfx $pe }
}

# ---- verify every file actually carries a signature --------------------------------
$bad = 0
foreach ($t in $pe) {
    $sig = Get-AuthenticodeSignature -FilePath $t
    if (-not $sig.SignerCertificate) { Write-Host "::error::Not signed: $t"; $bad++; continue }
    if ($backend -eq 'akv') {
        # ours is a public certificate: anything but a valid, timestamped signature must not ship
        if ($sig.Status -ne 'Valid') { Write-Host "::error::Signature is $($sig.Status): $t - $($sig.StatusMessage)"; $bad++; continue }
        if (-not $sig.TimeStamperCertificate) { Write-Host "::error::No timestamp: $t"; $bad++; continue }
    }
    Write-Host ("  signed  {0}  [{1}]  {2}" -f $sig.Status, $sig.SignerCertificate.Subject, (Split-Path $t -Leaf))
}
if ($bad -gt 0) { exit 1 }
Write-Host "All $($pe.Count) file(s) signed."
