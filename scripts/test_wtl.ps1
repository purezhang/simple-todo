$clPath = "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
$testFile = "D:\MyProject\ClaudeData\simple-todo\src\stdafx.cpp"

# Only include WTL path first
$includePaths = @(
    "D:\MyProject\ClaudeData\simple-todo\third_party\WTL"
)

$argList = "/EHsc /Od /Zi /MDd /std:c++17 /D_WIN32_WINNT=0x0601 /DUNICODE /D_UNICODE /utf-8 /c"
foreach ($path in $includePaths) {
    $argList += " /I `"$path`""
}
$argList += " `"$testFile`""

Write-Host "Testing with WTL include path only..."
Write-Host "Command: $clPath $argList"
Write-Host ""

$output = & $clPath $argList 2>&1
Write-Host $output
Write-Host ""
Write-Host "Exit code: $LASTEXITCODE"
