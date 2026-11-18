<#
Writes latest.json for a built installer and signs it.
Usage: .\sign-release.ps1 -Version 0.1.1 -BaseUrl https://github.com/<org>/<repo>/releases/download/v0.1.1 -Notes "What changed"
Upload Tideline-Setup-<version>.exe and latest.json to the release (latest.json must also be reachable at the
address the launcher checks: see updateManifestUrl in app\src\lib\Util\Brand.ts).
#>
param(
  [Parameter(Mandatory)][string]$Version,
  [Parameter(Mandatory)][string]$BaseUrl,
  [string]$Notes = ""
)
$ErrorActionPreference = "Stop"
$exe = Join-Path $PSScriptRoot "output\Tideline-Setup-$Version.exe"
if (-not (Test-Path $exe)) { throw "Build the installer first: $exe not found" }
$priv = Join-Path $env:USERPROFILE ".tideline-signing\update-private.pk8"
if (-not (Test-Path $priv)) { throw "No signing key. Run make-update-key.ps1 first." }
$ec = [System.Security.Cryptography.ECDsa]::Create()
$read = 0
$ec.ImportPkcs8PrivateKey([IO.File]::ReadAllBytes($priv), [ref]$read)
$sha = (Get-FileHash $exe -Algorithm SHA256).Hash.ToLower()
$url = "$($BaseUrl.TrimEnd('/'))/Tideline-Setup-$Version.exe"
$msg = "tideline-update|$Version|$sha|$url"
$sig = $ec.SignData([Text.Encoding]::UTF8.GetBytes($msg), [Security.Cryptography.HashAlgorithmName]::SHA256)  # 64 bytes r||s
$manifest = [ordered]@{ version = $Version; url = $url; sha256 = $sha; sig = ([BitConverter]::ToString($sig).Replace("-", "").ToLower()); notes = $Notes; size = (Get-Item $exe).Length }
$out = Join-Path $PSScriptRoot "output\latest.json"
($manifest | ConvertTo-Json) | Set-Content $out -Encoding utf8
Write-Host "Wrote $out`n$msg"
