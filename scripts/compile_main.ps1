# SimpleTodo Compile Script
# Usage: .\compile_main.ps1 [-Configuration Debug|Release] [-Rebuild]

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [switch]$Rebuild
)

# 1. Environment Setup
$vsPath = "D:\MyDevTools\Microsoft Visual Studio\18\Community"
$kitPath = "C:\Program Files (x86)\Windows Kits\10"
$kitVer = "10.0.26100.0"
$msvcVer = "14.50.35717"

$env:PATH = "$vsPath\VC\Tools\MSVC\$msvcVer\bin\Hostx64\x64;" + $env:PATH
$env:PATH = "$vsPath\Common7\IDE;" + $env:PATH
$env:PATH = "$kitPath\bin\$kitVer\x64;" + $env:PATH

$clExe = "cl.exe"
$rcExe = "rc.exe"

$projectRoot = $PSScriptRoot + "\.."
$srcDir = "$projectRoot\src"
$outDir = "$projectRoot\bin\x64\$Configuration"
$objDir = "$projectRoot\obj\x64\$Configuration"

# Include paths
$includePaths = @(
    "$srcDir",
    "$vsPath\VC\Tools\MSVC\$msvcVer\atlmfc\include",
    "$kitPath\Include\$kitVer\ucrt",
    "$kitPath\Include\$kitVer\shared",
    "$kitPath\Include\$kitVer\um",
    "$vsPath\VC\Tools\MSVC\$msvcVer\include"
)

$resFile = "$objDir\SimpleTodo.res"

# 2. Clean
if ($Rebuild) {
    Write-Host "Cleaning..."
    if (Test-Path $outDir) { Remove-Item -Path $outDir -Recurse -Force -ErrorAction SilentlyContinue }
    if (Test-Path $objDir) { Remove-Item -Path $objDir -Recurse -Force -ErrorAction SilentlyContinue }
}

if (!(Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir -Force | Out-Null }
if (!(Test-Path $objDir)) { New-Item -ItemType Directory -Path $objDir -Force | Out-Null }

# 3. Compile Resources
Write-Host "Compiling resources..."

$rcPath = "$kitPath\bin\$kitVer\x64\rc.exe"
$oldDir = Get-Location

Set-Location $objDir

$rcArgs = @("/r", "/d", "WIN32")
if ($Configuration -eq "Debug") { $rcArgs += "/d"; $rcArgs += "_DEBUG" }
else { $rcArgs += "/d"; $rcArgs += "NDEBUG" }

foreach ($path in $includePaths) {
    $rcArgs += "/i"
    $rcArgs += $path
}
$rcArgs += "/fo"
$rcArgs += $resFile
$rcArgs += "$srcDir\SimpleTodo.rc"

# 输出完整命令用于调试
$rcCmdStr = $rcArgs -join " "
Write-Host "RC Command: $rcPath $rcCmdStr"

# 调用资源编译器
$ErrorActionPreference = "Continue"
$rcOutput = & $rcPath $rcArgs 2>&1
$ErrorActionPreference = "SilentlyContinue"

foreach ($line in $rcOutput) {
    Write-Host $line
}

if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne $null) {
    Write-Host "ERROR: Resource compilation failed with code $LASTEXITCODE"
    Set-Location $oldDir
    exit 1
}

Set-Location $oldDir

# 4. Compile Source & Link
Write-Host "Compiling source..."
$exeFile = "$outDir\SimpleTodo.exe"

$clArgs = @(
    "/nologo", "/EHsc", "/std:c++17", "/D_WIN32_WINNT=0x0601", "/DUNICODE", "/D_UNICODE", "/utf-8",
    "/Fo$objDir\", "/Fe$exeFile"
)

if ($Configuration -eq "Debug") { $clArgs += "/Od", "/Zi", "/MDd", "/D_DEBUG" }
else { $clArgs += "/O2", "/MD", "/DNDEBUG" }

# Includes
foreach ($p in $includePaths) { $clArgs += "/I"; $clArgs += $p }
$clArgs += "/I"; $clArgs += "$projectRoot\third_party"
$clArgs += "/I"; $clArgs += "$projectRoot\third_party\SQLite"
$clArgs += "/I"; $clArgs += "$vsPath\VC\Tools\MSVC\$msvcVer\include"

# Sources
$sources = @(
    "$srcDir\AddTodoDlg.cpp",
    "$srcDir\MainFrm.cpp",
    "$srcDir\SimpleTodo.cpp",
    "$srcDir\SQLiteManager.cpp",
    "$srcDir\stdafx.cpp",
    "$srcDir\TodoListCtrl.cpp",
    "$srcDir\TodoModel.cpp",
    "$projectRoot\third_party\SQLite\sqlite3.c"
)
$clArgs += $sources

# Linker options
$clArgs += "/link"
$clArgs += "/LIBPATH:$vsPath\VC\Tools\MSVC\$msvcVer\lib\x64"
$clArgs += "/LIBPATH:$vsPath\VC\Tools\MSVC\$msvcVer\atlmfc\lib\x64"
$clArgs += "/LIBPATH:$kitPath\Lib\$kitVer\ucrt\x64"
$clArgs += "/LIBPATH:$kitPath\Lib\$kitVer\um\x64"
$clArgs += "user32.lib", "kernel32.lib", "gdi32.lib", "comctl32.lib", "comdlg32.lib", "uuid.lib", "advapi32.lib", "shell32.lib"
$clArgs += "/SUBSYSTEM:WINDOWS"
if ($Configuration -eq "Debug") { $clArgs += "/DEBUG" }
$clArgs += $resFile

# Execute CL
& $clExe $clArgs

# 嵌入清单文件
$manifestPath = "$srcDir\SimpleTodo.manifest"
$mtPath = "$kitPath\bin\$kitVer\x64\mt.exe"
if (Test-Path $manifestPath -and (Test-Path $exeFile)) {
    Write-Host "Embedding manifest..."
    $mtProc = Start-Process -FilePath $mtPath -ArgumentList "-manifest", $manifestPath, "-outputresource:$exeFile;1" -NoNewWindow -Wait -PassThru
    if ($mtProc.ExitCode -ne 0) {
        Write-Host "WARNING: Manifest embedding failed with code $($mtProc.ExitCode)"
    }
}

if (Test-Path $exeFile) {
    Write-Host ""
    Write-Host "SUCCESS: $exeFile"
    Get-Item $exeFile | Select-Object LastWriteTime, Length
} else {
    Write-Error "Compilation failed."
    exit 1
}
