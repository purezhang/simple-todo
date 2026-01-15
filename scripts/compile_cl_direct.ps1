$clPath = "D:/MyDevTools/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64/cl.exe"
$srcDir = "D:/MyProject/ClaudeData/simple-todo/src"
$thirdPartyDir = "D:/MyProject/ClaudeData/simple-todo/third_party"
$outputDir = "D:/MyProject/ClaudeData/simple-todo/bin/x64/Debug"
$outputExe = "$outputDir/SimpleTodo.exe"
$resFile = "$outputDir/SimpleTodo.res"

$cmd = "$clPath /EHsc /Od /Zi /MDd /std:c++17 /D_WIN32_WINNT=0x0601 /DUNICODE /D_UNICODE /utf-8 "
$cmd += "/I`"$thirdPartyDir/WTL`" /I`"$thirdPartyDir/SQLite`" /I`"$srcDir`" "
$cmd += "`"$srcDir/AddTodoDlg.cpp`" `"$srcDir/MainFrm.cpp`" `"$srcDir/SimpleTodo.cpp`" "
$cmd += "`"$srcDir/SQLiteManager.cpp`" `"$srcDir/stdafx.cpp`" `"$srcDir/TodoListCtrl.cpp`" "
$cmd += "`"$srcDir/TodoModel.cpp`" `"$thirdPartyDir/SQLite/sqlite3.c`" "
$cmd += "/link /OUT:`"$outputExe`" "
$cmd += "/LIBPATH:`"D:/MyDevTools/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/lib/x64`" "
$cmd += "/LIBPATH:`"D:/MyDevTools/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/atlmfc/lib/x64`" "
$cmd += "/LIBPATH:`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64`" "
$cmd += "/LIBPATH:`"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64`" "
$cmd += "user32.lib kernel32.lib gdi32.lib comctl32.lib comdlg32.lib uuid.lib advapi32.lib shell32.lib "
$cmd += "/SUBSYSTEM:WINDOWS /DEBUG `"$resFile`""

Write-Host "Compiling..."
$proc = Start-Process -FilePath $clPath -ArgumentList $cmd -NoNewWindow -Wait -PassThru
Write-Host "Exit code: $($proc.ExitCode)"

if (Test-Path $outputExe) {
    Get-Item $outputExe | Select-Object Name, LastWriteTime, Length
}
