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

      SSL.com eSigner - individuals, IV/OV certificate in their cloud (the chosen path)
        ESIGNER_USERNAME, ESIGNER_PASSWORD, ESIGNER_CREDENTIAL_ID, ESIGNER_TOTP_SECRET
        ESIGNER_SANDBOX=1 (optional) rehearses against SSL.com's sandbox with their demo account

      PFX file - any certificate you hold as a .pfx (testing, self-hosted)
        WIN_SIGN_PFX_BASE64, WIN_SIGN_PFX_PASSWORD

    With none of them set the script prints a notice and exits 0, so the
    pipeline keeps producing unsigned builds until the secrets exist.
    See docs/CODE-SIGNING.md.

.PARAMETER Status
    Print the backend that would be used (acs | esigner | pfx | none) and exit.
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

function Has([string[]] $names) {
    foreach ($n in $names) { if ([string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable($n))) { return $false } }
    return $true
}

function Detect-Backend {
    if (Has @('AZURE_TENANT_ID', 'AZURE_CLIENT_ID', 'AZURE_CLIENT_SECRET', 'ACS_ENDPOINT', 'ACS_ACCOUNT', 'ACS_PROFILE')) { return 'acs' }
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
    'esigner' { Sign-ESigner $pe }
    'pfx'     { Sign-Pfx $pe }
}

# ---- verify every file actually carries a signature --------------------------------
$bad = 0
foreach ($t in $pe) {
    $sig = Get-AuthenticodeSignature -FilePath $t
    if (-not $sig.SignerCertificate) { Write-Host "::error::Not signed: $t"; $bad++; continue }
    Write-Host ("  signed  {0}  [{1}]  {2}" -f $sig.Status, $sig.SignerCertificate.Subject, (Split-Path $t -Leaf))
}
if ($bad -gt 0) { exit 1 }
Write-Host "All $($pe.Count) file(s) signed."
