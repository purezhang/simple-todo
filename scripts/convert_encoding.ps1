# Check and convert file encoding to UTF-8
$basePath = "D:\MyProject\ClaudeData\simple-todo\SimpleTodo"
$files = Get-ChildItem -Path $basePath -Filter "*.cpp" -Recurse
$files += Get-ChildItem -Path $basePath -Filter "*.h" -Recurse

foreach ($file in $files) {
    $content = [System.IO.File]::ReadAllText($file.FullName, [System.Text.Encoding]::UTF8)
    [System.IO.File]::WriteAllText($file.FullName, $content, [System.Text.Encoding]::UTF8)
    Write-Host "Converted: $($file.Name)"
}

Write-Host "Done!"
