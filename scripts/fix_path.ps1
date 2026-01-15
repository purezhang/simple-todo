$clPath = "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
$srcDir = "D:\MyProject\ClaudeData\simple-todo\src"
$wtlDir = "D:\MyProject\ClaudeData\simple-todo\third_party\WTL"
$sqliteDir = "D:\MyProject\ClaudeData\simple-todo\third_party\SQLite"

# 使用 Join-Path 确保路径正确连接
$stdafxFile = Join-Path $srcDir "stdafx.cpp"

Write-Host "stdafxFile: '$stdafxFile'"

$args = @(
    "/EHsc", "/Od", "/Zi", "/MDd", "/std:c++17",
    "/D_WIN32_WINNT=0x0601", "/DUNICODE", "/D_UNICODE", "/utf-8", "/c",
    "/I", $wtlDir,
    "/I", $sqliteDir,
    "/I", $srcDir,
    "/I", "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\include",
    "/I", "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include",
    "/I", "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt",
    "/I", "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared",
    "/I", "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um",
    $stdafxFile
)

Write-Host ""
Write-Host "Arguments:"
$args | ForEach-Object { Write-Host "  [$_]" }

Write-Host ""
Write-Host "Running cl.exe..."

$output = & $clPath $args 2>&1 | Out-String
Write-Host $output

$exitCode = $LASTEXITCODE
Write-Host ""
Write-Host "Exit code: $exitCode"
