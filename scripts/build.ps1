# SimpleTodo Build Script
# Usage: .\build.ps1 [-Configuration Debug|Release] [-Rebuild] [-Quiet] [-Help]

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration,

    [switch]$Rebuild,

    [switch]$Quiet,

    [switch]$Help
)

# Get script root directory for relative paths
$scriptRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$srcDir = Join-Path $scriptRoot "src"
$thirdPartyDir = Join-Path $scriptRoot "third_party"

# Show help if explicitly requested OR if no configuration is specified
if ($Help -or [string]::IsNullOrWhiteSpace($Configuration)) {
    Write-Host "========================================"
    Write-Host "SimpleTodo Build Script"
    Write-Host "========================================"
    Write-Host ""
    Write-Host "Usage:"
    Write-Host "  .\build.ps1 -Configuration <Debug|Release> [-Rebuild] [-Quiet]"
    Write-Host ""
    Write-Host "Parameters:"
    Write-Host "  -Configuration    Build type (Debug or Release). REQUIRED."
    Write-Host "  -Rebuild         Clean output directory before compiling (Preserves Data)"
    Write-Host "  -Quiet           Suppress detailed compiler output"
    Write-Host "  -Help            Show this help message"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  # View Help (Default)"
    Write-Host "  .\build.ps1"
    Write-Host ""
    Write-Host "  # Build Release version"
    Write-Host "  .\build.ps1 -Configuration Release"
    Write-Host ""
    Write-Host "  # Rebuild Debug version (Clean + Build)"
    Write-Host "  .\build.ps1 -Configuration Debug -Rebuild"
    Write-Host ""
    Write-Host "  # Quiet build"
    Write-Host "  .\build.ps1 -Configuration Release -Quiet"
    Write-Host ""
    exit 0
}

# =============================================================================
# Helper Functions
# =============================================================================

function Find-LatestWindowsSdkVersion {
    param([string]$BasePath)

    if (-not (Test-Path $BasePath)) { return $null }

    $versions = Get-ChildItem $BasePath | Where-Object { $_.Name -match "^10\.0\.\d+\.0$" } | Sort-Object Name -Descending
    return $versions | Select-Object -First 1
}

function Find-MtExe {
    param([string]$WindowsKitsPath)

    if (-not (Test-Path $WindowsKitsPath)) { return $null }

    $binPath = Join-Path $WindowsKitsPath "bin"
    if (-not (Test-Path $binPath)) { return $null }

    $latestSdk = Find-LatestWindowsSdkVersion -BasePath $binPath
    if ($latestSdk) {
        $mtPath = Join-Path $latestSdk.FullName "x64\mt.exe"
        if (Test-Path $mtPath) {
            return $mtPath
        }
    }
    return $null
}

# =============================================================================
# Environment Setup
# =============================================================================

# Find Visual Studio
$vsWherePath = "${Env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsInfo = $null
$clPath = $null

if (Test-Path $vsWherePath) {
    $vsInfo = & $vsWherePath -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    if ($vsInfo) {
        $vcToolsPath = Join-Path $vsInfo "VC\Tools\MSVC"
        if (Test-Path $vcToolsPath) {
            $latestVcVersion = Get-ChildItem $vcToolsPath | Sort-Object Name -Descending | Select-Object -First 1
            if ($latestVcVersion) {
                $clPathCandidate = Join-Path $latestVcVersion.FullName "bin\Hostx64\x64\cl.exe"
                if (Test-Path $clPathCandidate) {
                    $clPath = $clPathCandidate
                    $env:PATH = "$($latestVcVersion.FullName)\bin\Hostx64\x64;" + $env:PATH
                    $env:PATH = "$vsInfo\Common7\IDE;" + $env:PATH
                }
            }
        }
    }
}

if (-not $clPath) {
    Write-Host "ERROR: Visual Studio C++ compiler (cl.exe) not found"
    exit 1
}

# Find Windows SDK
$windowsKitsPath = "C:\Program Files (x86)\Windows Kits\10"
$rcPath = $null

if (Test-Path $windowsKitsPath) {
    $binPath = Join-Path $windowsKitsPath "bin"
    if (Test-Path $binPath) {
        $latestSdk = Find-LatestWindowsSdkVersion -BasePath $binPath
        if ($latestSdk) {
            $rcPathCandidate = Join-Path $latestSdk.FullName "x64\rc.exe"
            if (Test-Path $rcPathCandidate) {
                $rcPath = $rcPathCandidate
                $env:PATH = "$($latestSdk.FullName)\x64;" + $env:PATH
            }
        }
    }
}

if (-not $rcPath) {
    # Fallback: try to find in PATH
    $rcPath = (Get-Command rc.exe -ErrorAction SilentlyContinue).Source
    if (-not $rcPath) {
        Write-Host "ERROR: Windows SDK Resource Compiler (rc.exe) not found"
        exit 1
    }
}

# Find mt.exe for manifest embedding
$mtPath = Find-MtExe -WindowsKitsPath $windowsKitsPath

# =============================================================================
# Build Configuration
# =============================================================================

$outputDir = Join-Path $scriptRoot "bin\x64\$Configuration"
$outputExe = Join-Path $outputDir "SimpleTodo.exe"
$resFile = Join-Path $outputDir "SimpleTodo.res"
$manifestPath = Join-Path $srcDir "SimpleTodo.exe.manifest"

# =============================================================================
# Pre-build Cleanup (Kill processes holding the exe)
# =============================================================================

# Kill any running SimpleTodo process to release file lock
Write-Host "Checking for running SimpleTodo processes..."
$processes = Get-Process SimpleTodo -ErrorAction SilentlyContinue
if ($processes) {
    Write-Host "Terminating running SimpleTodo processes..."
    $processes | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500
}

# Force delete the output exe if it exists and is locked
if (Test-Path $outputExe) {
    Write-Host "Attempting to release locked file: $outputExe"
    try {
        # Try to delete with multiple attempts
        for ($i = 1; $i -le 3; $i++) {
            try {
                Remove-Item -Path $outputExe -Force -ErrorAction Stop
                Write-Host "File released successfully."
                break
            }
            catch {
                if ($i -lt 3) {
                    Write-Host "Retry $i/3 in 1 second..."
                    Start-Sleep -Milliseconds 1000
                }
                else {
                    Write-Host "WARNING: Could not delete locked file. Build may fail."
                }
            }
        }
    }
    catch {
        Write-Host "WARNING: Failed to remove locked file: $_"
    }
}

# =============================================================================
# Setup Build Log
# =============================================================================

# Create log directory
$logDir = Join-Path $scriptRoot "bin\build-log"
if (-not (Test-Path $logDir)) {
    New-Item -ItemType Directory -Path $logDir | Out-Null
}

# Generate log filename with timestamp
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$logFile = Join-Path $logDir "simple-todo-$timestamp.log"

# Start logging
$logContent = @()
$logContent += "========================================"
$logContent += "Build started at $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
$logContent += "Configuration: $Configuration"
$logContent += "========================================"
$logContent += ""

Write-Host "Build log: $logFile"

# Helper function to add to log
function Add-Log {
    param([string]$Message)
    $script:logContent += $Message
    Write-Host $Message
}

# Rebuild / Clean logic
if ($Rebuild) {
    if (Test-Path $outputDir) {
        Add-Log "Cleaning output directory (preserving data): $outputDir"
        Get-ChildItem -Path $outputDir | Where-Object { $_.Name -ne "data" } | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
        Add-Log "Clean complete."
    }
}

if (!(Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# =============================================================================
# Generate Version Header
# =============================================================================

Write-Host ""
Add-Log "=== Version Info ==="

# Get version from git branch
$version = git branch --show-current
if ([string]::IsNullOrWhiteSpace($version)) {
    $version = "dev"
}
$buildDate = Get-Date -Format "yyyyMMdd"
$buildTime = Get-Date -Format "HHmmss"

# Generate clean version string: branch-yyyymmdd-hhmmss
$cleanVersion = "$version-$buildDate-$buildTime"

Add-Log "  Git branch:  $version"
Add-Log "  Build date:  $buildDate"
Add-Log "  Build time:  $buildTime"
Add-Log "  Version:     $cleanVersion"
Add-Log ""
Add-Log "=== Generating version header ==="

$versionHeaderPath = Join-Path $srcDir "version.h"

# Generate version.h content
# Numeric versions for RC file (default to 0 when using branch-based versioning)
$vMajor = 0
$vMinor = 0
$vPatch = 0
$vBuild = 0

# Add debug suffix for debug builds
$versionSuffix = ""
if ($Configuration -eq "Debug") {
    $versionSuffix = "-DEBUG"
}

$versionContent = @"
// Auto-generated by build.ps1 - DO NOT EDIT
#pragma once

// String versions
#define APP_VERSION_STRING L"$cleanVersion"
#define APP_BUILD_DATE L"$buildDate$buildTime"
#define APP_VERSION_FULL L"$cleanVersion$versionSuffix (build $buildDate-$buildTime)"

// Numeric versions for RC file
#define APP_VERSION_MAJOR $vMajor
#define APP_VERSION_MINOR $vMinor
#define APP_VERSION_PATCH $vPatch
#define APP_VERSION_BUILD $vBuild
#define APP_VERSION_CSV $vMajor,$vMinor,$vPatch,$vBuild
#define APP_VERSION_STR "$vMajor.$vMinor.$vPatch.$vBuild"
"@

# Write to file
$versionContent | Out-File -FilePath $versionHeaderPath -Encoding utf8 -Force

Add-Log "  Version file: $versionHeaderPath"
Add-Log "  Version string: $cleanVersion$versionSuffix"
Add-Log "=== Version header generated ==="
Add-Log ""

# =============================================================================
# Compile Resources
# =============================================================================

Add-Log "=== Starting compilation ==="
Write-Host "Compiling resources... ($Configuration)"

# Find latest SDK version for includes
$latestSdkInclude = Find-LatestWindowsSdkVersion -BasePath (Join-Path $windowsKitsPath "Include")
$latestSdkLib = Find-LatestWindowsSdkVersion -BasePath (Join-Path $windowsKitsPath "Lib")

$includePaths = @($srcDir)

if ($vsInfo) {
    $includePaths += Join-Path $vsInfo "VC\Tools\MSVC\$($latestVcVersion.Name)\include"
    $includePaths += Join-Path $vsInfo "VC\Tools\MSVC\$($latestVcVersion.Name)\atlmfc\include"
}

if ($latestSdkInclude) {
    $includePaths += Join-Path $latestSdkInclude.FullName "ucrt"
    $includePaths += Join-Path $latestSdkInclude.FullName "shared"
    $includePaths += Join-Path $latestSdkInclude.FullName "um"
}

$rcArgs = @("/r", "/d", "WIN32")
if ($Configuration -eq "Debug") {
    $rcArgs += "/d", "_DEBUG"
}
else {
    $rcArgs += "/d", "NDEBUG"
}

foreach ($path in $includePaths) {
    $rcArgs += "/i", $path
}

$rcArgs += "/fo", $resFile
$rcArgs += (Join-Path $srcDir "SimpleTodo.rc")

$rcOutput = & $rcPath $rcArgs 2>&1
if (-not $Quiet) {
    $rcOutput | Out-String | Write-Host
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Resource compilation failed with code $LASTEXITCODE"
    exit 1
}

if (-not (Test-Path $resFile)) {
    Write-Host "ERROR: Resource file not found!"
    exit 1
}

# =============================================================================
# Compile Source Files
# =============================================================================

Write-Host "Compiling source files... ($Configuration)"

$clArgs = @(
    "/EHsc", "/std:c++17", "/D_WIN32_WINNT=0x0601", "/DUNICODE", "/D_UNICODE", "/utf-8"
)

$clArgs += "/Fo$outputDir\"

if ($Configuration -eq "Debug") {
    $clArgs += "/Od", "/Zi", "/MTd", "/D_DEBUG"
}
else {
    $clArgs += "/O2", "/MT", "/DNDEBUG"
}

$includeDirs = @(
    $thirdPartyDir,
    "$thirdPartyDir\SQLite",
    $srcDir
)

if ($vsInfo) {
    $includeDirs += Join-Path $vsInfo "VC\Tools\MSVC\$($latestVcVersion.Name)\include"
    $includeDirs += Join-Path $vsInfo "VC\Tools\MSVC\$($latestVcVersion.Name)\atlmfc\include"
}

if ($latestSdkInclude) {
    $includeDirs += Join-Path $latestSdkInclude.FullName "ucrt"
    $includeDirs += Join-Path $latestSdkInclude.FullName "shared"
    $includeDirs += Join-Path $latestSdkInclude.FullName "um"
}

foreach ($path in $includeDirs) {
    $clArgs += "/I", $path
}

$sourceFiles = @(
    (Join-Path $srcDir "AddTodoDlg.cpp"),
    (Join-Path $srcDir "MainFrm.cpp"),
    (Join-Path $srcDir "SimpleTodo.cpp"),
    (Join-Path $srcDir "SQLiteManager.cpp"),
    (Join-Path $srcDir "stdafx.cpp"),
    (Join-Path $srcDir "TodoListCtrl.cpp"),
    (Join-Path $srcDir "TodoModel.cpp"),
    "$thirdPartyDir\SQLite\sqlite3.c"
)

foreach ($file in $sourceFiles) {
    $clArgs += $file
}

$clArgs += "/link"
$clArgs += "/OUT:$outputExe"

# Add library paths
if ($latestVcVersion) {
    $clArgs += "/LIBPATH:$($latestVcVersion.FullName)\lib\x64"
    $clArgs += "/LIBPATH:$($latestVcVersion.FullName)\atlmfc\lib\x64"
}

if ($latestSdkLib) {
    $clArgs += "/LIBPATH:$($latestSdkLib.FullName)\ucrt\x64"
    $clArgs += "/LIBPATH:$($latestSdkLib.FullName)\um\x64"
}

$clArgs += "user32.lib", "kernel32.lib", "gdi32.lib", "comctl32.lib", "comdlg32.lib", "uuid.lib", "advapi32.lib", "shell32.lib"
$clArgs += "/SUBSYSTEM:WINDOWS"

if ($Configuration -eq "Debug") {
    $clArgs += "/DEBUG"
}

$clArgs += $resFile

# Execute compiler
$clOutput = & $clPath $clArgs 2>&1
$clExitCode = $LASTEXITCODE

# Add compiler output to log
foreach ($line in $clOutput) {
    Add-Log $line
}

if ((-not (Test-Path $outputExe)) -or ($clExitCode -ne 0)) {
    # Save log file before exiting
    $logContent += ""
    $logContent += "========================================"
    $logContent += "BUILD FAILED"
    $logContent += "Exit code: $clExitCode"
    $logContent += "Output exe exists: $(Test-Path $outputExe)"
    $logContent += "========================================"
    $logContent | Out-File -FilePath $logFile -Encoding utf8

    Add-Log ""
    Add-Log "========================================"
    Add-Log "FAILURE: Compilation failed."
    Add-Log "Exit code: $clExitCode"
    Add-Log "Log saved to: $logFile"
    Add-Log "========================================"

    # Check if exe exists but is stale (>3 minutes old)
    if (Test-Path $outputExe) {
        $exeAge = (Get-Date) - (Get-Item $outputExe).LastWriteTime
        if ($exeAge.TotalMinutes -gt 3) {
            Add-Log ""
            Add-Log "WARNING: Output exe exists but is older than 3 minutes."
            Add-Log "Possible issues:"
            Add-Log "  1. Another process is holding the exe file"
            Add-Log "  2. Antivirus software may be blocking the file"
            Add-Log "  3. Close Windows Explorer preview handlers"
        }
    }

    exit 1
}

# =============================================================================
# Embed Manifest
# =============================================================================

if (Test-Path $manifestPath) {
    if ($mtPath) {
        Add-Log "Embedding manifest..."
        $mtOutput = & $mtPath -manifest $manifestPath -outputresource:"$outputExe;#1" 2>&1
        foreach ($line in $mtOutput) {
            Add-Log $line
        }
        if ($LASTEXITCODE -eq 0) {
            Add-Log "Manifest embedded successfully."
        }
        else {
            Add-Log "WARNING: Manifest embedding failed with code $LASTEXITCODE"
        }
    }
    else {
        Add-Log "WARNING: mt.exe not found, manifest not embedded."
    }
}

# =============================================================================
# Build Complete
# =============================================================================

# Save successful build log
$logContent += ""
$logContent += "========================================"
$logContent += "BUILD SUCCESSFUL"
$logContent += "Output: $outputExe"
$logContent += "========================================"
$logContent | Out-File -FilePath $logFile -Encoding utf8

Add-Log ""
Add-Log "========================================"
Add-Log "SUCCESS: $outputExe"
Add-Log "Log saved to: $logFile"
Add-Log "========================================"
Get-Item $outputExe | Select-Object Name, LastWriteTime, Length
