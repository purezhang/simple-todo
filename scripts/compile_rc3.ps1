$rcPath = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe"
$srcFile = "D:\MyProject\ClaudeData\simple-todo\src\SimpleTodo.rc"
$resFile = "D:\MyProject\ClaudeData\simple-todo\bin\x64\Debug\SimpleTodo.res"

$includePaths = @(
    "D:\MyProject\ClaudeData\simple-todo\src",
    "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared",
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um"
)

$includeArg = ($includePaths | ForEach-Object { "/i ""$_""" }) -join " "

# Try without /fo first - rc will generate SimpleTodo.res in the current directory
$argList = "/r /d WIN32 $includeArg ""$srcFile"""

Write-Host "Running: $rcPath $argList"
Write-Host ""

$process = Start-Process -FilePath $rcPath -ArgumentList $argList -NoNewWindow -Wait -PassThru -ErrorAction Stop

Write-Host "Exit code: $($process.ExitCode)"

# Check if SimpleTodo.res was created in the output directory
if (Test-Path $resFile) {
    Write-Host "SUCCESS: Resource compiled to $resFile"
} elseif (Test-Path "D:\MyProject\ClaudeData\simple-todo\bin\x64\Debug\SimpleTodo.res") {
    Write-Host "Resource compiled to default location, moving to target location..."
    Move-Item -Path "D:\MyProject\ClaudeData\simple-todo\bin\x64\Debug\SimpleTodo.res" -Destination $resFile -Force
    Write-Host "SUCCESS: Resource moved to $resFile"
} else {
    Write-Host "FAILED: Resource compilation failed"
    # List all res files
    Get-ChildItem -Path "D:\MyProject\ClaudeData\simple-todo\bin\x64\Debug\" -Filter *.res -Recurse
}
