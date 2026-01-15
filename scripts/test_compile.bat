@echo off
chcp 65001 >nul
setlocal

set clPath=D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe
set srcDir=D:\MyProject\ClaudeData\simple-todo\src

echo 编译测试...

"%clPath%" /EHsc /Od /Zi /MDd /std:c++17 /D_WIN32_WINNT=0x0601 /DUNICODE /D_UNICODE /utf-8 /c ^
    /I "D:\MyProject\ClaudeData\simple-todo\third_party\WTL" ^
    /I "D:\MyProject\ClaudeData\simple-todo\third_party\SQLite" ^
    /I "%srcDir%" ^
    /I "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\include" ^
    /I "D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include" ^
    /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt" ^
    /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" ^
    /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" ^
    "%srcDir%\stdafx.cpp"

echo 退出码: %ERRORLEVEL%
pause
