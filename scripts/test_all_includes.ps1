$clPath = "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
$testFile = "D:\MyProject\ClaudeData\simple-todo\src\stdafx.cpp"

# All include paths
$includePaths = @(
    "D:\MyProject\ClaudeData\simple-todo\third_party\WTL",
    "D:\MyProject\ClaudeData\simple-todo\third_party\SQLite",
    "D:\MyProject\ClaudeData\simple-todo\src",
    "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\include",
    "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um"
)

$argList = "/EHsc /Od /Zi /MDd /std:c++17 /D_WIN32_WINNT=0x0601 /DUNICODE /D_UNICODE /utf-8 /c"
foreach ($path in $includePaths) {
    $argList += " /I `"$path`""
}
$argList += " `"$testFile`""

Write-Host "Testing with all include paths..."
Write-Host "Command: $clPath $argList"
Write-Host ""

$output = & $clPath $argList 2>&1
Write-Host $output
Write-Host ""
Write-Host "Exit code: $LASTEXITCODE"
