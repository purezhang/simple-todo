@echo off
setlocal

taskkill /F /IM SimpleTodo.exe >nul 2>&1
rmdir /S /Q bin\x64\Debug >nul 2>&1
rmdir /S /Q obj\x64\Debug >nul 2>&1
mkdir bin\x64\Debug
mkdir obj\x64\Debug

set "VS_PATH=D:\MyDevTools\Microsoft Visual Studio\18\Community"
set "KIT_PATH=C:\Program Files (x86)\Windows Kits\10"
set "MSVC_VER=14.50.35717"
set "KIT_VER=10.0.26100.0"

set "PATH=%VS_PATH%\VC\Tools\MSVC\%MSVC_VER%\bin\Hostx64\x64;%KIT_PATH%\bin\%KIT_VER%\x64;%PATH%"

set "INC_ATLMFC=%VS_PATH%\VC\Tools\MSVC\%MSVC_VER%\atlmfc\include"
set "INC_UCRT=%KIT_PATH%\Include\%KIT_VER%\ucrt"
set "INC_SHARED=%KIT_PATH%\Include\%KIT_VER%\shared"
set "INC_UM=%KIT_PATH%\Include\%KIT_VER%\um"
set "INC_MSVC=%VS_PATH%\VC\Tools\MSVC\%MSVC_VER%\include"

set "LIB_MSVC=%VS_PATH%\VC\Tools\MSVC\%MSVC_VER%\lib\x64"
set "LIB_ATLMFC=%VS_PATH%\VC\Tools\MSVC\%MSVC_VER%\atlmfc\lib\x64"
set "LIB_UCRT=%KIT_PATH%\Lib\%KIT_VER%\ucrt\x64"
set "LIB_UM=%KIT_PATH%\Lib\%KIT_VER%\um\x64"

echo Compiling resources...
rc.exe /r /d WIN32 /d _DEBUG /i "src" /i "%INC_ATLMFC%" /i "%INC_UCRT%" /i "%INC_SHARED%" /i "%INC_UM%" /fo "obj\x64\Debug\SimpleTodo.res" "src\SimpleTodo.rc"
if %errorlevel% neq 0 exit /b %errorlevel%

echo Compiling source...
cl.exe /nologo /EHsc /std:c++17 /D_WIN32_WINNT=0x0601 /DUNICODE /D_UNICODE /utf-8 /Od /Zi /MDd /D_DEBUG /I "src" /I "third_party" /I "third_party\SQLite" /I "%INC_MSVC%" /I "%INC_ATLMFC%" /I "%INC_UCRT%" /I "%INC_SHARED%" /I "%INC_UM%" /Fo"obj\x64\Debug\" /Fe"bin\x64\Debug\SimpleTodo.exe" "src\AddTodoDlg.cpp" "src\MainFrm.cpp" "src\SimpleTodo.cpp" "src\SQLiteManager.cpp" "src\stdafx.cpp" "src\TodoListCtrl.cpp" "src\TodoModel.cpp" "third_party\SQLite\sqlite3.c" /link /LIBPATH:"%LIB_MSVC%" /LIBPATH:"%LIB_ATLMFC%" /LIBPATH:"%LIB_UCRT%" /LIBPATH:"%LIB_UM%" user32.lib kernel32.lib gdi32.lib comctl32.lib comdlg32.lib uuid.lib advapi32.lib shell32.lib /SUBSYSTEM:WINDOWS /DEBUG "obj\x64\Debug\SimpleTodo.res"
if %errorlevel% neq 0 exit /b %errorlevel%

echo SUCCESS
endlocal
