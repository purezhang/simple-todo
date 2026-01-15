@echo off
chcp 65001 >nul
echo ============================================
echo SimpleTodo 编译脚本
echo ============================================

cd /d "%~dp0.."

set PATH=D:\MyDevTools\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;%PATH%
set PATH=D:\MyDevTools\Microsoft Visual Studio\18\Common7\IDE;%PATH%

echo 开始编译...
msbuild SimpleTodo.vcxproj /p:Configuration=Debug /p:Platform=x64 /t:Rebuild /v:normal

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo 编译成功！
    echo 文件: %~dp0..\bin\x64\Debug\SimpleTodo.exe
    echo ============================================
) else (
    echo.
    echo ============================================
    echo 编译失败！
    echo ============================================
)

pause
