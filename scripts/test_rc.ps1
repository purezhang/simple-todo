$rcPath = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe"
$srcFile = "D:\MyProject\ClaudeData\simple-todo\src\SimpleTodo.rc"
$resFile = "D:\MyProject\ClaudeData\simple-todo\bin\x64\Debug\SimpleTodo.res"

Write-Host "rc.exe path: $rcPath"
Write-Host "Source: $srcFile"
Write-Host "Output: $resFile"
Write-Host ""

# Check if files exist
Write-Host "Checking files..."
Write-Host "rc.exe exists: $(Test-Path $rcPath)"
Write-Host "Source exists: $(Test-Path $srcFile)"

# Try running rc.exe with help first
Write-Host ""
Write-Host "Testing rc.exe..."
& $rcPath /? 2>&1 | Select-Object -First 5
