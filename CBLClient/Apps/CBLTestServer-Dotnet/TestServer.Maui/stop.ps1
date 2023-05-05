$ChildPID = Get-Process -ProcessName TestServer.Maui | Select-Object -Expand Id
Stop-Process $ChildPID