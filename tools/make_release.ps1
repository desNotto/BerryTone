# BerryTone release script (Windows).
# 1. Builds the Release configuration.
# 2. Copies the VST3 into <ReleaseRoot>\BerryTone-v<version> as ONE file
#    (BerryTone.vst3 = the plugin DLL). Some DAWs do not find the bundle folder.
# 3. Makes BerryTone-v<version>-win64.zip (the plugin updater downloads this file).
# 4. With -Publish: makes the git tag and the GitHub release (needs gh).
#    The release starts the GitHub workflow "macOS build": it adds BerryTone-v<version>-mac.zip.
# 5. With -WaitMac: waits for that file (up to 40 min) and downloads it into the release folder.
#
#   powershell -ExecutionPolicy Bypass -File tools\make_release.ps1
#   powershell -ExecutionPolicy Bypass -File tools\make_release.ps1 -Publish
param(
    [string]$ReleaseRoot = (Join-Path $PSScriptRoot "..\..\Releases"),
    [switch]$Publish,
    [switch]$WaitMac,
    [string]$Notes = ""
)
$ErrorActionPreference = "Stop"
$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repo

# version from CMakeLists.txt
$version = (Select-String -Path CMakeLists.txt -Pattern 'set\(BERRY_VERSION ([0-9.]+)\)').Matches[0].Groups[1].Value
Write-Host "BerryTone v$version"

# cmake (PATH or Visual Studio Build Tools)
$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vs = & $vswhere -latest -products * -property installationPath
    $cmake = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
}

# JUCE makes the Windows version resource only once (it depends on the icon only).
# Delete it, so the file version is the new version.
Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $repo "build\BerryTone_artefacts\JuceLibraryCode\BerryTone_resources.rc")
& $cmake -B build -G "Visual Studio 17 2022" -A x64 | Out-Null
& $cmake --build build --config Release --parallel 8
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

$art = Join-Path $repo "build\BerryTone_artefacts\Release"
$out = Join-Path (New-Item -ItemType Directory -Force -Path $ReleaseRoot).FullName "BerryTone-v$version"
if (Test-Path $out) { Remove-Item $out -Recurse -Force }
New-Item -ItemType Directory -Path $out | Out-Null

Copy-Item (Join-Path $art "VST3\BerryTone.vst3\Contents\x86_64-win\BerryTone.vst3") (Join-Path $out "BerryTone.vst3")
@"
BerryTone v$version - by desNotto
https://github.com/desNotto/BerryTone

Install
- Copy the file BerryTone.vst3 to C:\Program Files\Common Files\VST3
- Rescan the plugins in your DAW.
Next updates: click "Update to ..." at the bottom of the plugin window.
"@ | Out-File -Encoding utf8 (Join-Path $out "README.txt")

$zip = Join-Path $out "BerryTone-v$version-win64.zip"
Compress-Archive -Path (Join-Path $out "BerryTone.vst3"), (Join-Path $out "README.txt") -DestinationPath $zip -Force
Write-Host "Release folder: $out"

if ($Publish) {
    if (git status --porcelain) { throw "Commit your changes before you publish." }
    git tag -a "v$version" -m "BerryTone v$version" 2>$null
    git push origin "v$version"
    if (-not $Notes) { $Notes = "BerryTone v$version" }
    gh release create "v$version" $zip --repo desNotto/BerryTone --title "BerryTone v$version" --notes $Notes
    if ($LASTEXITCODE -ne 0) { throw "gh release create failed" }
    Write-Host "Published v$version on GitHub. The macOS build starts now (GitHub Actions)."
}

if ($WaitMac) {
    $macZip = "BerryTone-v$version-mac.zip"
    Write-Host "Waiting for $macZip ..."
    $deadline = (Get-Date).AddMinutes(40)
    while ((Get-Date) -lt $deadline) {
        $names = gh release view "v$version" --repo desNotto/BerryTone --json assets -q '.assets[].name'
        if ($names -contains $macZip) {
            gh release download "v$version" --repo desNotto/BerryTone -p $macZip -D $out --clobber
            Write-Host "Downloaded $macZip into $out"
            break
        }
        Start-Sleep -Seconds 30
    }
    if (-not (Test-Path (Join-Path $out $macZip))) { throw "$macZip is not in the release. Check GitHub Actions." }
}
