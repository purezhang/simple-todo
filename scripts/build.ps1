# SimpleTodo Build Script
# Usage: .\build.ps1 [-Configuration Debug|Release] [-Rebuild] [-Help]

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration,

    [switch]$Rebuild,

    [switch]$Help
)

# Show help if explicitly requested OR if no configuration is specified
if ($Help -or [string]::IsNullOrWhiteSpace($Configuration)) {
    Write-Host "========================================"
    Write-Host "SimpleTodo Build Script"
    Write-Host "========================================"
    Write-Host ""
    Write-Host "Usage:"
    Write-Host "  .\build.ps1 -Configuration <Debug|Release> [-Rebuild]"
    Write-Host ""
    Write-Host "Parameters:"
    Write-Host "  -Configuration    Build type (Debug or Release). REQUIRED."
    Write-Host "  -Rebuild          Clean output directory before compiling (Preserves Data)"
    Write-Host "  -Help             Show this help message"
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
    exit 0
}

# Environment Setup
$env:PATH = "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;" + $env:PATH
$env:PATH = "D:\MyDevTools\Microsoft Visual Studio\18\Community\Common7\IDE;" + $env:PATH
$env:PATH = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64;" + $env:PATH

$clPath = "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
$rcPath = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe"
$srcDir = "D:\MyProject\ClaudeData\simple-todo\src"
$thirdPartyDir = "D:\MyProject\ClaudeData\simple-todo\third_party"

$outputDir = "D:\MyProject\ClaudeData\simple-todo\bin\x64\$Configuration"
$outputExe = Join-Path $outputDir "SimpleTodo.exe"
$resFile = Join-Path $outputDir "SimpleTodo.res"
$objDir = $outputDir + "\"

# Rebuild / Clean logic
if ($Rebuild) {
    if (Test-Path $outputDir) {
        Write-Host "Cleaning output directory (preserving data): $outputDir"
        # Remove all items EXCEPT 'data' directory
        Get-ChildItem -Path $outputDir | Where-Object { $_.Name -ne "data" } | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "Clean complete."
    }
}

if (!(Test-Path $outputDir)) { New-Item -ItemType Directory -Path $outputDir | Out-Null }

# Compile Resources
Write-Host "Compiling resources... ($Configuration)"
$oldDir = Get-Location
Set-Location $outputDir

$includePaths = @(
    $srcDir,
    "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um"
)

$rcArgs = @("/r", "/d", "WIN32")
if ($Configuration -eq "Debug") { $rcArgs += "/d"; $rcArgs += "_DEBUG" }
else { $rcArgs += "/d"; $rcArgs += "NDEBUG" }

foreach ($path in $includePaths) {
    $rcArgs += "/i"; $rcArgs += $path
}
$rcArgs += "/fo"; $rcArgs += $resFile
$rcArgs += (Join-Path $srcDir "SimpleTodo.rc")

& $rcPath $rcArgs | Out-String | Write-Host
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Resource compilation failed with code $LASTEXITCODE"
    Set-Location $oldDir
    exit 1
}

Set-Location $oldDir

if (!(Test-Path $resFile)) {
    Write-Host "ERROR: Resource file not found!"
    exit 1
}

# Compile Source
Write-Host "Compiling source files... ($Configuration)"

$clArgs = @(
    "/EHsc", "/std:c++17", "/D_WIN32_WINNT=0x0601", "/DUNICODE", "/D_UNICODE", "/utf-8"
)

$clArgs += "/Fo$objDir"

if ($Configuration -eq "Debug") {
    $clArgs += "/Od", "/Zi", "/MDd", "/D_DEBUG"
}
else {
    $clArgs += "/O2", "/MD", "/DNDEBUG"
}

foreach ($path in @(
        $thirdPartyDir,
        "$thirdPartyDir\SQLite",
        $srcDir,
        "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\include",
        "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include",
        "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt",
        "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared",
        "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um"
    )) {
    $clArgs += "/I"; $clArgs += $path
}

foreach ($file in @(
        (Join-Path $srcDir "AddTodoDlg.cpp"),
        (Join-Path $srcDir "MainFrm.cpp"),
        (Join-Path $srcDir "SimpleTodo.cpp"),
        (Join-Path $srcDir "SQLiteManager.cpp"),
        (Join-Path $srcDir "stdafx.cpp"),
        (Join-Path $srcDir "TodoListCtrl.cpp"),
        (Join-Path $srcDir "TodoModel.cpp"),
        "$thirdPartyDir\SQLite\sqlite3.c"
    )) {
    $clArgs += $file
}

$clArgs += "/link"
$clArgs += "/OUT:$outputExe"
$clArgs += "/LIBPATH:D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\lib\x64"
$clArgs += "/LIBPATH:D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\lib\x64"
$clArgs += "/LIBPATH:C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64"
$clArgs += "/LIBPATH:C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64"
$clArgs += "user32.lib", "kernel32.lib", "gdi32.lib", "comctl32.lib", "comdlg32.lib", "uuid.lib", "advapi32.lib", "shell32.lib"
$clArgs += "/SUBSYSTEM:WINDOWS"

if ($Configuration -eq "Debug") { $clArgs += "/DEBUG" }

$clArgs += $resFile

# Exec compiler
& $clPath $clArgs | Out-String | Write-Host

if (Test-Path $outputExe) {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "SUCCESS: $outputExe"
    Write-Host "========================================"
    Get-Item $outputExe | Select-Object Name, LastWriteTime, Length
}
else {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "FAILURE: Compilation failed."
    Write-Host "========================================"
    exit 1
}
