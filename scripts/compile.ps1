$output = & "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" *>&1
Push-Location "D:\MyProject\ClaudeData\simple-todo"
$env:chcp = "65001"
$compileResult = cl /EHsc /O2 /W4 /std:c++17 /D_WIN32_WINNT=0x0601 /utf-8 /I"D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include" /I"Sqlite" /I"WTL" /I"." SimpleTodo\*.cpp /link /OUT:bin\x64\Release\SimpleTodo.exe user32.lib kernel32.lib gdi32.lib comctl32.lib sqlite3.lib /SUBSYSTEM:WINDOWS 2>&1
Pop-Location
Write-Host $compileResult
