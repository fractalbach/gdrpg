#Requires -Version 5.1
<#
.SYNOPSIS
    Configure, build, test, and optionally run the gdrpg demo.

.EXAMPLE
    .\build.ps1
    .\build.ps1 -Config Debug
    .\build.ps1 -Test -Demo
    .\build.ps1 -Clean
    .\build.ps1 -Extension
#>
[CmdletBinding()]
param(
    [ValidateSet("Release", "Debug", "RelWithDebInfo")]
    [string]$Config = "Release",

    [string]$BuildDir = "build",

    [switch]$Clean,
    [switch]$Test,
    [switch]$Demo,
    [switch]$Extension,
    [switch]$ConfigureOnly,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

if ($Help) {
    Write-Host @"
gdrpg build script

Usage:
  .\build.ps1 [-Config Release|Debug] [-BuildDir build] [-Clean] [-Test] [-Demo] [-Extension]

Options:
  -Config <name>   Build type (default: Release)
  -BuildDir <dir>  Out-of-source build directory (default: build)
  -Clean           Delete the build directory first
  -Test            Run ctest after building
  -Demo            Run the standalone demo after building
  -Extension       Also build the Godot GDExtension
  -ConfigureOnly   Only run CMake configure
  -Help            Show this help

Examples:
  .\build.ps1
  .\build.ps1 -Test -Demo
  .\build.ps1 -Config Debug -Clean
"@
    exit 0
}

function Find-CMake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $candidates = @(
        "${env:ProgramFiles}\CMake\bin\cmake.exe",
        "${env:ProgramFiles(x86)}\CMake\bin\cmake.exe",
        "${env:LOCALAPPDATA}\Programs\CMake\bin\cmake.exe"
    )

    # Visual Studio bundled CMake (VS 2022 / 2026+)
    $vsRoots = @(
        "${env:ProgramFiles}\Microsoft Visual Studio",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio"
    )
    foreach ($root in $vsRoots) {
        if (-not (Test-Path $root)) { continue }
        $found = Get-ChildItem $root -Recurse -Filter cmake.exe -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\CMake\\CMake\\bin\\cmake\.exe$' } |
            Select-Object -First 1
        if ($found) { return $found.FullName }
    }

    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

function Find-VsDevCmd {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
        if ($installPath) {
            $bat = Join-Path $installPath "Common7\Tools\VsDevCmd.bat"
            if (Test-Path $bat) { return $bat }
        }
    }

    $roots = @(
        "${env:ProgramFiles}\Microsoft Visual Studio",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio"
    )
    foreach ($root in $roots) {
        if (-not (Test-Path $root)) { continue }
        $found = Get-ChildItem $root -Recurse -Filter VsDevCmd.bat -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($found) { return $found.FullName }
    }
    return $null
}

function Import-VsDevEnvironment {
    param([string]$VsDevCmd)
    Write-Host "==> Loading Visual Studio developer environment"
    $temp = [System.IO.Path]::GetTempFileName()
    cmd /c "`"$VsDevCmd`" -arch=x64 -host_arch=x64 >nul && set" | Out-File -FilePath $temp -Encoding ascii
    Get-Content $temp | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
        }
    }
    Remove-Item $temp -ErrorAction SilentlyContinue
}

function Invoke-Native {
    param([string]$FilePath, [string[]]$ArgumentList)
    Write-Host "==> $FilePath $($ArgumentList -join ' ')"
    & $FilePath @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed ($LASTEXITCODE): $FilePath $($ArgumentList -join ' ')"
    }
}

# --- main ---

$cmake = Find-CMake
if (-not $cmake) {
    Write-Error "cmake not found. Install CMake or Visual Studio with C++ tools."
}

# Prefer Ninja if available after VS env; otherwise let CMake pick a generator.
$vsDev = Find-VsDevCmd
$needVsEnv = $false
if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    if ($vsDev) { $needVsEnv = $true }
}
if ($needVsEnv) {
    Import-VsDevEnvironment -VsDevCmd $vsDev
    # Re-resolve cmake in case PATH changed
    $resolved = Find-CMake
    if ($resolved) { $cmake = $resolved }
}

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "==> Cleaning $BuildDir"
    Remove-Item -Recurse -Force $BuildDir
}

$extFlag = if ($Extension) { "ON" } else { "OFF" }
$generatorArgs = @()
if (Get-Command ninja -ErrorAction SilentlyContinue) {
    $generatorArgs = @("-G", "Ninja")
}

$configureArgs = @(
    "-B", $BuildDir
) + $generatorArgs + @(
    "-DCMAKE_BUILD_TYPE=$Config",
    "-DGDRPG_BUILD_DEMO=ON",
    "-DGDRPG_BUILD_TESTS=ON",
    "-DGDRPG_BUILD_EXTENSION=$extFlag"
)

Invoke-Native -FilePath $cmake -ArgumentList $configureArgs

if ($ConfigureOnly) {
    Write-Host "==> Configure complete."
    exit 0
}

Invoke-Native -FilePath $cmake -ArgumentList @("--build", $BuildDir, "--config", $Config)

if ($Test) {
    $ctest = Join-Path (Split-Path $cmake -Parent) "ctest.exe"
    if (-not (Test-Path $ctest)) {
        $ctestCmd = Get-Command ctest -ErrorAction SilentlyContinue
        if ($ctestCmd) { $ctest = $ctestCmd.Source } else { $ctest = "ctest" }
    }
    Invoke-Native -FilePath $ctest -ArgumentList @(
        "--test-dir", $BuildDir, "--output-on-failure", "-C", $Config
    )
}

if ($Demo) {
    $candidates = @(
        (Join-Path $BuildDir "demo\gdrpg_demo.exe"),
        (Join-Path $BuildDir "demo\gdrpg_demo"),
        (Join-Path $BuildDir "gdrpg_demo.exe"),
        (Join-Path $BuildDir "gdrpg_demo")
    )
    $demoExe = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $demoExe) {
        throw "Demo executable not found under $BuildDir"
    }
    Write-Host "==> Running demo: $demoExe"
    & $demoExe (Join-Path $PSScriptRoot "data")
    if ($LASTEXITCODE -ne 0) {
        throw "Demo exited with code $LASTEXITCODE"
    }
}

Write-Host "==> Done."
