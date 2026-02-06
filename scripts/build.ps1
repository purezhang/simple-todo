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

# Rebuild / Clean logic
if ($Rebuild) {
    if (Test-Path $outputDir) {
        Write-Host "Cleaning output directory (preserving data): $outputDir"
        Get-ChildItem -Path $outputDir | Where-Object { $_.Name -ne "data" } | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "Clean complete."
    }
}

if (!(Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# =============================================================================
# Compile Resources
# =============================================================================

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
} else {
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
} else {
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
if (-not $Quiet) {
    $clOutput | Out-String | Write-Host
}

if (-not (Test-Path $outputExe)) {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "FAILURE: Compilation failed."
    Write-Host "========================================"
    exit 1
}

# =============================================================================
# Embed Manifest
# =============================================================================

if (Test-Path $manifestPath) {
    if ($mtPath) {
        if (-not $Quiet) {
            Write-Host "Embedding manifest..."
        }
        $mtOutput = & $mtPath -manifest $manifestPath -outputresource:"$outputExe;#1" 2>&1
        if ($LASTEXITCODE -eq 0) {
            if (-not $Quiet) {
                Write-Host "Manifest embedded successfully."
            }
        } else {
            Write-Host "WARNING: Manifest embedding failed with code $LASTEXITCODE"
        }
    } elseif (-not $Quiet) {
        Write-Host "WARNING: mt.exe not found, manifest not embedded."
    }
}

# =============================================================================
# Build Complete
# =============================================================================

Write-Host ""
Write-Host "========================================"
Write-Host "SUCCESS: $outputExe"
Write-Host "========================================"
Get-Item $outputExe | Select-Object Name, LastWriteTime, Length
