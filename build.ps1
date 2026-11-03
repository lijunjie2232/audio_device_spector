# Builds audio_device_spector with CMake (Ninja preferred, MSVC fallback).
#   .\build.ps1            build only
#   .\build.ps1 -Run       build and launch
#   .\build.ps1 -Generator "Visual Studio 17 2022"   override generator

param(
    [switch]$Run,
    [string]$Generator = "",
    [string]$Config = "Release"
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$outDir = Join-Path $root 'build'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# Locate vcvars64.bat (vswhere first, then a filesystem scan).
$vcvars = $null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    if ($vsPath) {
        $candidate = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
        if (Test-Path $candidate) { $vcvars = $candidate }
    }
}
if (-not $vcvars) {
    $vcvars = Get-ChildItem 'C:\Program Files\Microsoft Visual Studio', 'C:\Program Files (x86)\Microsoft Visual Studio' -Directory -ErrorAction SilentlyContinue |
        ForEach-Object { Get-ChildItem $_.FullName -Directory -ErrorAction SilentlyContinue } |
        ForEach-Object { Join-Path $_.FullName 'VC\Auxiliary\Build\vcvars64.bat' } |
        Where-Object { Test-Path $_ } |
        Select-Object -First 1
}
if (-not $vcvars) {
    throw 'vcvars64.bat not found. Install Visual Studio (or Build Tools) with the C++ workload and Windows SDK.'
}

# Ensure ninja is available; auto-download if missing.
function Ensure-Ninja {
    if (Get-Command ninja -ErrorAction SilentlyContinue) { return }
    $toolsDir = Join-Path $outDir 'tools'
    $ninjaExe = Join-Path $toolsDir 'ninja.exe'
    if (-not (Test-Path $ninjaExe)) {
        New-Item -ItemType Directory -Force -Path $toolsDir | Out-Null
        $url = 'https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip'
        $zip = Join-Path $toolsDir 'ninja-win.zip'
        Write-Host "Downloading ninja..."
        Invoke-WebRequest -Uri $url -OutFile $zip
        Expand-Archive -Path $zip -DestinationPath $toolsDir -Force
        Remove-Item $zip -Force
    }
    $env:PATH = "$toolsDir;$env:PATH"
}
Ensure-Ninja

if ([string]::IsNullOrWhiteSpace($Generator)) {
    if (Get-Command ninja -ErrorAction SilentlyContinue) {
        $Generator = 'Ninja'
    } else {
        $Generator = 'Visual Studio 17 2022'
    }
}

# Auto-detect Windows SDK if missing (best-effort via vswhere packages).
$sdkInclude = 'C:\Program Files (x86)\Windows Kits\10\Include'
if (-not (Test-Path $sdkInclude)) {
    Write-Warning "Windows SDK not found at $sdkInclude. Install via Visual Studio Installer."
}

$configureCmd = 'call "{0}" >nul && cmake -S "{1}" -B "{2}" -G "{3}" -DCMAKE_BUILD_TYPE={4}' -f $vcvars, $root, $outDir, $Generator, $Config
& cmd.exe /d /c $configureCmd
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed with exit code $LASTEXITCODE" }

$buildCmd = 'call "{0}" >nul && cmake --build "{1}" --config {2}' -f $vcvars, $outDir, $Config
& cmd.exe /d /c $buildCmd
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }

$exe = Join-Path $outDir 'audio_output_monitor.exe'
if (-not (Test-Path $exe)) {
    # Ninja single-config outputs here; VS multi-config may nest
    $exe = Get-ChildItem $outDir -Recurse -Filter 'audio_output_monitor.exe' | Select-Object -First 1 -ExpandProperty FullName
}
Write-Host "Built: $exe"
if ($Run) { Start-Process $exe }
