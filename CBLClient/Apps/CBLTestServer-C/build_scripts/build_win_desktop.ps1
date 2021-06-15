param(
    [Parameter(Mandatory=$true)][string]$Version,
    [Parameter(Mandatory=$true)][string]$BuildNum
)

#$ErrorActionPreference="Stop"
$DOWNLOAD_DIR="$PSScriptRoot\..\downloaded"
$BUILD_DIR="$PSScriptRoot\..\build"
$ZIPS_DIR="$PSScriptRoot\..\zips"

Remove-Item -Recurse -Force -ErrorAction Ignore $DOWNLOAD_DIR
New-Item -ItemType Directory $DOWNLOAD_DIR

$ZIP_FILENAME="couchbase-lite-c-windows-x64-$Version-$BuildNum-enterprise.zip"
Invoke-WebRequest http://latestbuilds.service.couchbase.com/builds/latestbuilds/couchbase-lite-c/${Version}/${BuildNum}/${ZIP_FILENAME} -OutFile "$DOWNLOAD_DIR\$ZIP_FILENAME"
Push-Location $DOWNLOAD_DIR
7z x -y $ZIP_FILENAME
Remove-Item $ZIP_FILENAME
Pop-Location

New-Item -ErrorAction Ignore -ItemType Directory $BUILD_DIR
Push-Location $BUILD_DIR

try {
    & "C:\Program Files\CMake\bin\cmake.exe" -G "Visual Studio 15 2017" -A x64 -DCMAKE_PREFIX_PATH="${DOWNLOAD_DIR}" -DCMAKE_BUILD_TYPE=Release ..
    if($LASTEXITCODE -ne 0) {
        throw "Cmake failed!"
    } 

    & "C:\Program Files\CMake\bin\cmake.exe" --build . --target install --config Release --parallel 12
    if($LASTEXITCODE -ne 0) {
        throw "Build failed!"
    } 

    Copy-Item "$DOWNLOAD_DIR\bin\CouchbaseLiteC.dll" out\bin
    Push-Location out\bin
    7za a -tzip -mx9 $ZIPS_DIR\testserver_windesktop_x64.zip testserver.exe zlib.dll CouchbaseLiteC.dll
    Pop-Location
} finally {
    Pop-Location
}