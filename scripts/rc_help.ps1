$rcPath = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe"

Write-Host "RC Help:"
$process = Start-Process -FilePath $rcPath -ArgumentList "/?" -NoNewWindow -Wait -PassThru -ErrorAction Stop
