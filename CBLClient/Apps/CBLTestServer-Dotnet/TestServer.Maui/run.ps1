param (
    [Parameter()][string]$AppDirectory
)

$PackageFullName="146a2da2-5da8-42e7-a7ca-6b26cd59f0e4_1.0.0.1_x64__se6zyberqjyhc"
$LaunchName="146a2da2-5da8-42e7-a7ca-6b26cd59f0e4_1.0.0.1_x64__se6zyberqjyhc!App"
if(-Not $AppDirectory) {
    $AppDirectory = $PSScriptRoot
}

Remove-AppxPackage $PackageFullName -ErrorAction Ignore
Push-Location $AppDirectory
try {
    .\Add-AppDevPackage.ps1 -Force
} finally {
    Pop-Location
}

Invoke-Expression "start shell:AppsFolder\$LaunchName"
