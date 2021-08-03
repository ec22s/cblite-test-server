param (
    [string]$Version,
    [switch]$Community
)

function Modify-Packages {
    $filename = $args[0]
    $ver = $args[1]
    $community = $args[2]

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

Push-Location $PSScriptRoot

$fullVersion = Calculate-Version

try {
    Modify-Packages "$PSScriptRoot\TestServer.Framework.csproj" $fullVersion $Community
    Modify-Packages "$PSScriptRoot\..\TestServer\TestServer.csproj" $fullVersion $Community

    Push-Location ..\TestServer
    dotnet restore
    if($LASTEXITCODE -ne 0) {
        Write-Error "Restore failed for TestServer"
        exit 1
    }

    Pop-Location

    dotnet restore
    if($LASTEXITCODE -ne 0) {
        Write-Error "Restore failed for TestServer.Framework"
        exit 1
    }

    dotnet publish -c Release
    if($LASTEXITCODE -ne 0) {
        Write-Error "Publish failed for TestServer.Framework"
        exit 1
    }

    if(-Not (Test-Path "zips")) {
        New-Item -ItemType Directory "zips"
    }

    if(Test-Path "zips\TestServer.Framework.zip") {
        Remove-Item "zips\TestServer.Framework.zip"
    }
    
    $ZipPath = Resolve-Path ".\zips"

    Push-Location bin\Release\framework4.61\publish
    try {
        7z a -r ${ZipPath}\TestServer.Framework.zip *
    } finally {
        Pop-Location
    }
} finally {
    Pop-Location
}