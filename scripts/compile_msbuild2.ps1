$vsPath = "D:\MyDevTools\Microsoft Visual Studio\18"
$msbuildPath = "$vsPath\Community\MSBuild\Current\Bin\MSBuild.exe"
$solutionPath = "D:\MyProject\ClaudeData\simple-todo\SimpleTodo.sln"

Write-Host "========================================"
Write-Host "SimpleTodo 编译脚本 (MSBuild)"
Write-Host "========================================"
Write-Host "MSBuild: $msbuildPath"
Write-Host "解决方案: $solutionPath"
Write-Host ""

# 设置环境
$env:PATH = "$vsPath\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;$env:PATH"
$env:PATH = "$vsPath\Community\Common7\IDE;$env:PATH"

# 运行编译
$process = Start-Process -FilePath $msbuildPath -ArgumentList "`"$solutionPath`"", "/p:Configuration=Debug", "/p:Platform=x64", "/t:Rebuild", "/v:detailed" -NoNewWindow -Wait -PassThru -ErrorAction Stop

$exitCode = $process.ExitCode
Write-Host ""
Write-Host "========================================"
Write-Host "退出码: $exitCode"
Write-Host "========================================"

if ($exitCode -eq 0) {
    Write-Host ""
    Write-Host "编译成功!"
    Write-Host "文件: D:\MyProject\ClaudeData\simple-todo\bin\x64\Debug\SimpleTodo.exe"
} else {
    Write-Host ""
    Write-Host "编译失败!"
}

exit $exitCode
