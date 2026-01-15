$clPath = "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
$rcPath = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe"
$srcDir = "D:\MyProject\ClaudeData\simple-todo\src"
$thirdPartyDir = "D:\MyProject\ClaudeData\simple-todo\third_party"
$outputDir = "D:\MyProject\ClaudeData\simple-todo\bin\x64\Debug"
$outputExe = Join-Path $outputDir "SimpleTodo.exe"
$resFile = Join-Path $outputDir "SimpleTodo.res"

if (!(Test-Path $outputDir)) { New-Item -ItemType Directory -Path $outputDir | Out-Null }

Write-Host "编译资源文件..."

# 编译资源文件
Write-Host "编译资源..."
$oldDir = Get-Location
Set-Location $outputDir

$includePaths = @(
    $srcDir,
    "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include",
    "C:\Program Files (x86)\Windows Kits\10.0.261\Include\1000.0\ucrt",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um"
)

$rcArgs = @("/r", "/d", "WIN32")
foreach ($path in $includePaths) {
    $rcArgs += "/i"
    $rcArgs += $path
}
$rcArgs += (Join-Path $srcDir "SimpleTodo.rc")

Write-Host "资源编译器命令: $rcPath $($rcArgs -join ' ')"
$rcOutput = & $rcPath $rcArgs 2>&1 | Out-String
Write-Host $rcOutput

Set-Location $oldDir

# 检查资源文件
if (Test-Path $resFile) {
    Write-Host "资源编译成功: $resFile"
} else {
    Write-Host "资源编译失败!"
    Write-Host $rcOutput
    exit 1
}

# 编译源文件
Write-Host "编译中..."

# 构建编译参数数组
$clArgs = @(
    "/EHsc", "/Od", "/Zi", "/MDd", "/std:c++17",
    "/D_WIN32_WINNT=0x0601", "/DUNICODE", "/D_UNICODE", "/utf-8"
)

# 添加 include 路径 - 注意: third_party 包含 WTL 子目录
foreach ($path in @(
    $thirdPartyDir,           # WTL 子目录在 third_party 下
    "$thirdPartyDir\SQLite",
    $srcDir,                  # src 目录包含 afxres.h 存根
    "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\include",
    "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um"
)) {
    $clArgs += "/I"
    $clArgs += $path
}

# 添加源文件
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

# 添加链接选项
$clArgs += "/link"
$clArgs += "/OUT:$outputExe"
$clArgs += "/LIBPATH:D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\lib\x64"
$clArgs += "/LIBPATH:D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\lib\x64"
$clArgs += "/LIBPATH:C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64"
$clArgs += "/LIBPATH:C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64"
$clArgs += "user32.lib", "kernel32.lib", "gdi32.lib", "comctl32.lib", "comdlg32.lib", "uuid.lib", "advapi32.lib", "shell32.lib"
$clArgs += "/SUBSYSTEM:WINDOWS", "/DEBUG"
$clArgs += $resFile

Write-Host ""
Write-Host "运行编译器: $clPath"

$output = & $clPath $clArgs 2>&1 | Out-String
Write-Host $output

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
    exit 1
}
