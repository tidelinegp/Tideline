<#
Builds Tideline-Setup-<version>.exe.

Prerequisites (all already set up on the development PC):
  - the launcher built and installed with cmake (its install folder is the source of the launcher files)
  - the dashboard app packaged with PyInstaller (tideline-web\dist\tideline-web)
  - a Temurin 17 JRE zip (the private copy of Java that ships with Tideline)
  - Inno Setup 6 (ISCC.exe)

Usage:  .\build-installer.ps1 -Version 0.1.0
User data lives in %APPDATA%\tideline and %APPDATA%\tideline-web, never in the install folder, so installing or
updating never touches anyone's logins, settings or flipping history.
#>
param(
  [string]$Version = "0.1.0",
  [string]$LauncherDir = "C:\Users\bowse\tideline",
  [string]$DashboardDist = "C:\Users\bowse\BoltDev\tideline-web\dist\tideline-web",
  [string]$JreZip = "",
  [string]$Iscc = ""
)
$ErrorActionPreference = "Stop"
$here = $PSScriptRoot
$repo = Split-Path $here
$stage = Join-Path $here "stage"

if (-not $Iscc) {
  $Iscc = @("C:\Program Files (x86)\Inno Setup 6\ISCC.exe", "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $Iscc) { throw "Inno Setup (ISCC.exe) not found" }
if (-not $JreZip) {
  $JreZip = Get-ChildItem "$env:TEMP\claude" -Recurse -Filter "OpenJDK17U-jre_x64_windows_hotspot_*.zip" -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
}
if (-not $JreZip -or -not (Test-Path $JreZip)) { throw "Temurin 17 JRE zip not found: pass -JreZip" }

Write-Host "Staging files..."
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory $stage | Out-Null

# the launcher (without the dashboard folder, which is copied fresh below, and without the private Java)
Get-ChildItem $LauncherDir -Force | Where-Object { $_.Name -notin @("web", "jre") } | Copy-Item -Destination $stage -Recurse -Force

# the dashboard app
Copy-Item $DashboardDist (Join-Path $stage "web") -Recurse -Force

# the private Java
$tmp = Join-Path $env:TEMP "tideline-jre-extract"
if (Test-Path $tmp) { Remove-Item $tmp -Recurse -Force }
Expand-Archive $JreZip -DestinationPath $tmp
$inner = (Get-ChildItem $tmp -Directory | Select-Object -First 1).FullName
Move-Item $inner (Join-Path $stage "jre")
Remove-Item $tmp -Recurse -Force

# licence and credits travel with the program
foreach ($doc in "LICENCE", "NOTICE.md", "README.md") { Copy-Item (Join-Path $repo $doc) $stage -Force }

$size = [math]::Round((Get-ChildItem $stage -Recurse -File | Measure-Object Length -Sum).Sum / 1MB)
Write-Host "Staged $size MB. Compiling installer $Version..."
Push-Location $here
& $Iscc "/DAppVersion=$Version" "tideline.iss"
if ($LASTEXITCODE -ne 0) { throw "ISCC failed" }
Pop-Location

$exe = Join-Path $here "output\Tideline-Setup-$Version.exe"
$hash = (Get-FileHash $exe -Algorithm SHA256).Hash.ToLower()
Set-Content -Path "$exe.sha256" -Value "$hash  Tideline-Setup-$Version.exe" -NoNewline
Write-Host ("Built {0} ({1} MB)`nSHA-256 {2}" -f $exe, [math]::Round((Get-Item $exe).Length / 1MB), $hash)
