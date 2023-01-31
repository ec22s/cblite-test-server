param (
    [Parameter()][string]$AppDirectory
)

$PackageFullName="2d440d2f-7627-19ac-40fa-2dac74696c6c_1.0.0.0_x64__75cr2b68sm664"
$LaunchName="2d440d2f-7627-19ac-40fa-2dac74696c6c_75cr2b68sm664!App"
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
