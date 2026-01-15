@echo off
chcp 65001 >nul
echo ============================================
echo SimpleTodo 编译脚本
echo ============================================

rem 设置环境
set PATH=D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;%PATH%
set PATH=D:\MyDevTools\Microsoft Visual Studio\18\Community\Common7\IDE;%PATH%
set INCLUDE=D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\include;D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um
set LIB=D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\lib\x64;D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64

rem 删除旧的 obj 文件
echo 清理旧文件...
cd /d "%~dp0.."
del /q *.obj 2>nul
del /q *.pdb 2>nul
del /q *.idb 2>nul

rem 创建输出目录
if not exist bin\x64\Debug mkdir bin\x64\Debug

rem 编译
echo 编译中...
"D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe" /EHsc /Od /Zi /MDd /std:c++17 /D_WIN32_WINNT=0x0601 /DUNICODE /D_UNICODE /utf-8 /I"D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include" /I"third_party\SQLite" /I"third_party\WTL" /I"src" src\*.cpp third_party\SQLite\sqlite3.c /link /OUT:"bin\x64\Debug\SimpleTodo.exe" user32.lib kernel32.lib gdi32.lib comctl32.lib comdlg32.lib uuid.lib advapi32.lib shell32.lib /SUBSYSTEM:WINDOWS /DEBUG

rem 检查结果
if exist bin\x64\Debug\SimpleTodo.exe (
    echo.
    echo ============================================
    echo 编译成功！
    echo 文件: bin\x64\Debug\SimpleTodo.exe
    echo ============================================
) else (
    echo.
    echo ============================================
    echo 编译失败！
    echo ============================================
)

pause
