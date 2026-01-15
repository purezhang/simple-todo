$msbuildPath = "D:/MyDevTools/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe"
$solutionPath = "D:/MyProject/ClaudeData/simple-todo/SimpleTodo.sln"

Write-Host "使用 MSBuild 编译..."
Write-Host "解决方案: $solutionPath"

$process = Start-Process -FilePath $msbuildPath -ArgumentList "`"$solutionPath`"", "/p:Configuration=Debug", "/p:Platform=x64", "/t:Rebuild", "/v:normal" -NoNewWindow -Wait -PassThru

$exitCode = $process.ExitCode
Write-Host "Exit Code: $exitCode"

if ($exitCode -eq 0) {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "编译成功!"
    Write-Host "文件: D:/MyProject/ClaudeData/simple-todo/bin/x64/Debug/SimpleTodo.exe"
    Write-Host "========================================"
} else {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "编译失败，退出码: $exitCode"
    Write-Host "========================================"
}

exit $exitCode
