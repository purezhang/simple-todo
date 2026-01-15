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

$includeArg = ($includePaths | ForEach-Object { "/I ""$_""" }) -join " "

$argList = """$srcFile"" /d WIN32 /fo ""$resFile"" $includeArg"

Write-Host "Running: $rcPath $argList"
Write-Host ""

$process = Start-Process -FilePath $rcPath -ArgumentList $argList -NoNewWindow -Wait -PassThru -ErrorAction Stop

Write-Host "Exit code: $($process.ExitCode)"
