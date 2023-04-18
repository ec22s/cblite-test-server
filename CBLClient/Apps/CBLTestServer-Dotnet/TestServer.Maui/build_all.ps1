param (
    [Parameter()][string]$Version,
    [switch]$Community
)

function Modify-Packages {
    $filename = $args[0]
    $ver = $args[1]
    $community = $args[2]

    if(-Not $OnWindows) {
        $filename = $filename.Replace("\", "/")
    }

    $content = [System.IO.File]::ReadAllLines($filename)
    $checkNextLine = $false
    for($i = 0; $i -lt $content.Length; $i++) {
        $line = $content[$i]
        $isMatch = $line -match ".*?<PackageReference Include=`"Couchbase\.Lite(.*?)`""
        if($isMatch) {
            $oldPackageName = $matches[1]
            $packageName = $oldPackageName.Replace(".Enterprise", "")
            if(-Not $community) {
                $packageName = ".Enterprise" + $packageName;
            }

            $isMatch = $line -match ".*?Version=`"(.*?)`""
            if($isMatch) {
                $oldVersion = $matches[1]
                $line = $line.Replace("Couchbase.Lite$oldPackageName", "Couchbase.Lite$packageName").Replace($oldVersion, $ver)
            } else {
                $checkNextLine = $true
            }
            
            $content[$i] = $line
        } elseif($checkNextLine) {
            $isMatch = $line -match ".*?<Version>(.*?)</Version>"
            if($isMatch) {
                $oldVersion = $matches[1]
                $line = $line.Replace($oldVersion, $ver)
                $checkNextLine = $false
                $content[$i] = $line
            } else {
                $checkNextLine = !$line.Contains("</PackageReference>")
            }
        }
    }

    [System.IO.File]::WriteAllLines($filename, $content)
}

function Calculate-Version {
    $version_to_use = (($Version, $env:VERSION -ne $null) -ne '')[0]
    if($version_to_use -eq '' -or !$version_to_use) {
        throw "Version not defined for this script!  Either pass it in as -Version or define an environment variable named VERSION"
    }

    if($version_to_use.Contains("-")) {
        return $version_to_use
    }

    return $version_to_use + "-b*"
}

function Build-WinUI {
    Write-Host ""
    Write-Host "  ************************"
    Write-Host "  **** Building WinUI ****"
    Write-Host "  ************************"
    Write-Host ""

    dotnet publish -f net6.0-windows10.0.19041.0 -c Release TestServer.Maui.csproj
    Push-Location ".\bin\Release\net6.0-windows10.0.19041.0\win10-x64\AppPackages\TestServer.Maui_1.0.0.1_Test\"
    try {
        7z a -r ${ZipPath}\TestServer.Maui.WinUI.zip *
        7z a ${ZipPath}\TestServer.Maui.WinUI.zip $PSScriptRoot\run.ps1
        7z a ${ZipPath}\TestServer.Maui.WinUI.zip $PSScriptRoot\stop.ps1
    } finally {
        Pop-Location
    }
}

function Build-Android {
    Write-Host ""
    Write-Host "  **************************"
    Write-Host "  **** Building Android ****"
    Write-Host "  **************************"
    Write-Host ""

    dotnet publish -f net6.0-android -c Release TestServer.Maui.csproj
    Push-Location ".\bin\Release\net6.0-android\publish"
    Copy-Item *.apk $ZipPath\TestServer.Maui.Android.apk
}

function Build-Ios {
    Write-Host ""
    Write-Host "  **********************"
    Write-Host "  **** Building iOS ****"
    Write-Host "  **********************"
    Write-Host ""

    dotnet build -f net6.0-ios -c Release TestServer.Maui.csproj
    dotnet build -f net6.0-ios -c Release -r ios-arm64 TestServer.Maui.csproj
    Push-Location ".\bin\Release\net6.0-ios\iossimulator-x64"
    zip -r $ZipPath/TestServer.Maui.iOS.zip TestServer.Maui.app
    Pop-Location

    Push-Location ".\bin\Release\net6.0-ios\ios-arm64"
    zip -r $ZipPath/TestServer.Maui.iOS-Device.zip TestServer.Maui.app
    Pop-Location
}

function Build-Mac {
    Write-Host ""
    Write-Host "  *************************"
    Write-Host "  **** Building macOS ****"
    Write-Host "  *************************"
    Write-Host ""

    dotnet build -f net6.0-maccatalyst -c Release TestServer.Maui.csproj
    Push-Location ".\bin\Release\net6.0-maccatalyst\maccatalyst-x64"
    zip -r $ZipPath/TestServer.Maui.macOS.zip TestServer.Maui.app
    Pop-Location
}

$OnWindows = $PSVersionTable.PSVersion.Major -lt 6 -or $IsWindows
if(-Not $OnWindows -and -Not $IsMacOS) {
    throw "Script not supported on Linux"
}



Push-Location $PSScriptRoot
$fullVersion = Calculate-Version
if(-Not (Test-Path "zips")) {
    New-Item -ItemType Directory "zips" 
}

$ZipPath = Resolve-Path "zips"

try {
    Modify-Packages "$PSScriptRoot\..\TestServer\TestServer.csproj" $fullVersion $Community
    dotnet restore
    if($LASTEXITCODE -ne 0) {
        Write-Error "Restore failed for TestServer.Maui"
        exit 1
    }

    if($OnWindows) {
        Build-WinUI
        Build-Android
    } else {
        Build-Ios
        Build-Mac
    }

    
} finally {
    Pop-Location
}
