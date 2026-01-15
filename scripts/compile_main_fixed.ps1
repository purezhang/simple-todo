$env:PATH = "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;" + $env:PATH
$env:PATH = "D:\MyDevTools\Microsoft Visual Studio\18\Community\Common7\IDE;" + $env:PATH
$env:PATH = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64;" + $env:PATH

$clPath = "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
$rcPath = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe"
$srcDir = "D:\MyProject\ClaudeData\simple-todo\src"
$thirdPartyDir = "D:\MyProject\ClaudeData\simple-todo\third_party"
$outputDir = "D:\MyProject\ClaudeData\simple-todo\bin\x64\Debug"
$outputExe = "$outputDir\SimpleTodo.exe"
$resFile = "$outputDir\SimpleTodo.res"

if (!(Test-Path $outputDir)) { New-Item -ItemType Directory -Path $outputDir | Out-Null }

Write-Host "编译资源文件..."

# 编译资源文件
Write-Host "编译资源..."
$oldDir = Get-Location
Set-Location $outputDir

$includePaths = @(
    "$srcDir",
    "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um"
)

$includeArg = ($includePaths | ForEach-Object { "/i ""$_""" }) -join " "

$rcCmd = "$rcPath /r /d WIN32 $includeArg ""$srcDir\SimpleTodo.rc"""
Invoke-Expression $rcCmd 2>&1

Set-Location $oldDir

# 检查资源文件
if (Test-Path $resFile) {
    Write-Host "资源编译成功: $resFile"
} else {
    Write-Host "资源编译失败!"
    exit 1
}

# 编译源文件
Write-Host "编译中..."

$cmd = "$clPath /EHsc /Od /Zi /MDd /std:c++17 /D_WIN32_WINNT=0x0601 /DUNICODE /D_UNICODE /utf-8 "
$cmd += "/I ""$thirdPartyDir\WTL"" /I ""$thirdPartyDir\SQLite"" /I ""$srcDir"" "
$cmd += "/I ""D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\include"" "
$cmd += "/I ""D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include"" "
$cmd += "/I ""C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt"" "
$cmd += "/I ""C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared"" "
$cmd += "/I ""C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um"" "
$cmd += """$srcDir\AddTodoDlg.cpp"" ""$srcDir\MainFrm.cpp"" ""$srcDir\SimpleTodo.cpp"" "
$cmd += """$srcDir\SQLiteManager.cpp"" ""$srcDir\stdafx.cpp"" ""$srcDir\TodoListCtrl.cpp"" "
$cmd += """$srcDir\TodoModel.cpp"" ""$thirdPartyDir\SQLite\sqlite3.c"" "
$cmd += "/link /OUT:""$outputExe"" "
$cmd += "/LIBPATH:""D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\lib\x64"" "
$cmd += "/LIBPATH:""D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\lib\x64"" "
$cmd += "/LIBPATH:""C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64"" "
$cmd += "/LIBPATH:""C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64"" "
$cmd += "user32.lib kernel32.lib gdi32.lib comctl32.lib comdlg32.lib uuid.lib advapi32.lib shell32.lib "
$cmd += "/SUBSYSTEM:WINDOWS /DEBUG ""$resFile"""

Invoke-Expression $cmd 2>&1 | Tee-Object -FilePath "D:\MyProject\ClaudeData\simple-todo\compile_output.txt"

if (Test-Path $outputExe) {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "编译成功!"
    Write-Host "文件: $outputExe"
    Write-Host "========================================"
    Get-Item $outputExe | Select-Object Name, LastWriteTime, Length
} else {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "编译失败!"
    Write-Host "========================================"
}
