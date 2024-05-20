param (
    [string]$Version,
    [string]$VectorSearchVersion,
    [switch]$Community
)

function Modify-Packages {
    $filename = $args[0]
    $ver = $args[1]
    $vsver = $args[2]
    $community = $args[3]

    $enterprisePackageString = ".Enterprise"

    $content = [System.IO.File]::ReadAllLines($filename)
    $checkNextLine = $false
    for($i = 0; $i -lt $content.Length; $i++) {
        $line = $content[$i]
        $isMatch = $line -match "<PackageReference Include=`"Couchbase\.Lite(.*?)`""
        if($isMatch) {
            $oldPackageName = $matches[1]
            if ($oldPackageName -eq $enterprisePackageString) {
                $packageName = $oldPackageName.Replace($enterprisePackageString, "")
                if(-Not $community) {
                    $packageName = $enterprisePackageString + $packageName;
                }
            }

            $isMatch = $line -match ".*?Version=`"(.*?)`""
            if($isMatch) {
                $oldVersion = $matches[1]
                switch ($oldPackageName) {
                    $enterprisePackageString {
                        $line = $line.Replace("Couchbase.Lite$oldPackageName", "Couchbase.Lite$packageName").Replace($oldVersion, $ver)
                    }
                    ".VectorSearch" {
                        $line = $line.Replace($oldVersion, $vsver)
                    }
                }
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

# Vector search version should be mandatory, so we don't cases where a build was built against an unintended version.
function Get-Vector-Search-Version {
    $vectorSearchVersion = (($VectorSearchVersion, $env:VECTOR_SEARCH_VERSION -ne $null) -ne '')[0]
    if($vectorSearchVersion -eq '' -or !$vectorSearchVersion) {
        throw "VectorSearchVersion not defined for this script!  Either pass it in as -VECTOR_SEARCH_VERSION or define an environment variable named VERSION"
    }
    return $vectorSearchVersion
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
$vectorSearchVersion = Get-Vector-Search-Version

try {
    Modify-Packages "$PSScriptRoot\TestServer.NetCore.csproj" $fullVersion $vectorSearchVersion $Community
    Modify-Packages "$PSScriptRoot\..\TestServer\TestServer.csproj" $fullVersion $vectorSearchVersion $Community

    Push-Location ..\TestServer
    dotnet restore
    if($LASTEXITCODE -ne 0) {
        Write-Error "Restore failed for TestServer"
        exit 1
    }

    Pop-Location

    dotnet restore
    if($LASTEXITCODE -ne 0) {
        Write-Error "Restore failed for TestServer.NetCore"
        exit 1
    }

    dotnet publish -c Release -f net6.0
    if($LASTEXITCODE -ne 0) {
        Write-Error "Publish failed for TestServer.NetCore"
        exit 1
    }

    if(-Not (Test-Path "zips")) {
        New-Item -ItemType Directory "zips"
    }

    if(Test-Path "zips\TestServer.NetCore.zip") {
        Remove-Item "zips\TestServer.NetCore.zip"
    }
    
    $ZipPath = Resolve-Path ".\zips"

    Push-Location bin\Release\net6.0\publish
    try {
        7z a -r ${ZipPath}\TestServer.NetCore.zip *
    } finally {
        Pop-Location
    }
} finally {
    Pop-Location
}
