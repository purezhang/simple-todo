@echo off
chcp 65001 >nul
echo ============================================
echo SimpleTodo 完整编译脚本（包含资源）
echo ============================================

setlocal

rem 设置环境
set PATH=D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;%PATH%
set PATH=D:\MyDevTools\Microsoft Visual Studio\18\Community\Common7\IDE;%PATH%
set PATH=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64;%PATH%

rem 设置 include 和 lib
set INCLUDE=D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\include;D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um
set LIB=D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\lib\x64;D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\atlmfc\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64

rem 切换到项目目录
cd /d "%~dp0.."

rem 删除旧的 obj 文件
echo 清理旧文件...
del /q bin\x64\Debug\*.exe 2>nul
del /q bin\x64\Debug\*.pdb 2>nul
del /q bin\x64\Debug\*.res 2>nul
del /q obj\x64\Debug\*.obj 2>nul

rem 创建输出目录
if not exist bin\x64\Debug mkdir bin\x64\Debug
if not exist obj\x64\Debug mkdir obj\x64\Debug

rem 编译资源文件
echo 编译资源文件...
cd src
..\..\..\..\..\..\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe SimpleTodo.rc /i ".." /d WIN32
cd ..

if not exist bin\x64\Debug\SimpleTodo.res (
    echo 资源编译失败，尝试使用简化资源...
    copy src\SimpleTodo.rc bin\x64\Debug\SimpleTodo.rc >nul
    cd bin\x64\Debug
    ..\..\..\..\..\..\..\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe SimpleTodo.rc /d WIN32
    cd ..\..\..
    if exist bin\x64\Debug\SimpleTodo.res (
        echo 资源编译成功
    ) else (
        echo 警告：资源编译失败，将使用无菜单版本
    )
)

rem 编译源文件
echo 编译中...
"D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe" /EHsc /Od /Zi /MDd /std:c++17 /D_WIN32_WINNT=0x0601 /DUNICODE /D_UNICODE /utf-8 /I"third_party\WTL" /I"third_party\SQLite" /I"src" src\*.cpp third_party\SQLite\sqlite3.c /link /OUT:"bin\x64\Debug\SimpleTodo.exe" user32.lib kernel32.lib gdi32.lib comctl32.lib comdlg32.lib uuid.lib advapi32.lib shell32.lib /SUBSYSTEM:WINDOWS /DEBUG

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

endlocal
pause
