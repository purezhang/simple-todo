$clPath = "D:/MyDevTools/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64/cl.exe"
$srcDir = "D:/MyProject/ClaudeData/simple-todo/src"
$thirdPartyDir = "D:/MyProject/ClaudeData/simple-todo/third_party"
$outputDir = "D:/MyProject/ClaudeData/simple-todo/bin/x64/Debug"
$outputExe = "$outputDir/SimpleTodo.exe"
$resFile = "$outputDir/SimpleTodo.res"
$tempExe = "$outputDir/SimpleTodo_new.exe"

# Build source file list
$sourceFiles = @(
    "$srcDir/AddTodoDlg.cpp",
    "$srcDir/MainFrm.cpp",
    "$srcDir/SimpleTodo.cpp",
    "$srcDir/SQLiteManager.cpp",
    "$srcDir/stdafx.cpp",
    "$srcDir/TodoListCtrl.cpp",
    "$srcDir/TodoModel.cpp",
    "$thirdPartyDir/SQLite/sqlite3.c"
)

# Build include paths
$includePaths = @(
    "$thirdPartyDir/WTL",
    "$thirdPartyDir/SQLite",
    "$srcDir",
    "D:/MyDevTools/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/include",
    "D:/MyDevTools/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/atlmfc/include",
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/ucrt",
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared",
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um"
)

# Build lib paths
$libPaths = @(
    "D:/MyDevTools/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/lib/x64",
    "D:/MyDevTools/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/atlmfc/lib/x64",
    "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64",
    "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"
)

# Build arguments
$argList = "/EHsc /Od /Zi /MDd /std:c++17 /D_WIN32_WINNT=0x0601 /DUNICODE /D_UNICODE /utf-8"

foreach ($path in $includePaths) {
    $argList += " /I `"$path`""
}

foreach ($file in $sourceFiles) {
    $argList += " `"$file`""
}

$argList += " /link /OUT:`"$tempExe`""

foreach ($path in $libPaths) {
    $argList += " /LIBPATH:`"$path`""
}

$argList += " user32.lib kernel32.lib gdi32.lib comctl32.lib comdlg32.lib uuid.lib advapi32.lib shell32.lib"
$argList += " /SUBSYSTEM:WINDOWS /DEBUG"
$argList += " `"$resFile`""

Write-Host "Compiling..."
Write-Host "Command: $clPath $argList"
Write-Host ""

$process = Start-Process -FilePath $clPath -ArgumentList $argList -NoNewWindow -Wait -PassThru -ErrorAction Stop

Write-Host ""
Write-Host "Exit code: $($process.ExitCode)"

if ($process.ExitCode -eq 0 -and (Test-Path $tempExe)) {
    # Delete old file if locked, rename new file
    try {
        if (Test-Path $outputExe) {
            Remove-Item $outputExe -Force -ErrorAction Stop
        }
    } catch {
        Write-Host "Warning: Could not delete old file"
    }

    try {
        Move-Item $tempExe $outputExe -Force
    } catch {
        Write-Host "Warning: Could not rename file"
    }

    Get-Item $outputExe | Select-Object Name, LastWriteTime, Length
} else {
    Write-Host "Compilation failed!"
}
