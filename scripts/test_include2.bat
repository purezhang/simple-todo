@echo off
chcp 65001 >nul
echo 测试编译...

set clPath=D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe
set testFile=D:\MyProject\ClaudeData\simple-todo\src\stdafx.cpp

%clPath% /EHsc /Od /Zi /MDd /std:c++17 /D_WIN32_WINNT=0x0601 /DUNICODE /D_UNICODE /utf-8 /c /I "D:\MyProject\ClaudeData\simple-todo\third_party\WTL" /I "D:\MyProject\ClaudeData\simple-todo\third_party\SQLite" /I "D:\MyProject\ClaudeData\simple-todo\src" /I "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\include" /I "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" "%testFile%"

echo 退出码: %ERRORLEVEL% > test_result.txt
pause
