#!/bin/bash -ex

# Global define
VERSION=${1}
BLD_NUM=${2}

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
DOWNLOAD_DIR=$SCRIPT_DIR/../ios/Frameworks
ZIPS_DIR=$SCRIPT_DIR/../zips
BUILD_SIM_FOLDER="build_sim"
BUILD_DEVICE_FOLDER="build_device"

# rm -rf $DOWNLOAD_DIR 2> /dev/null
# mkdir -p $DOWNLOAD_DIR

# pushd $DOWNLOAD_DIR
# ZIP_FILENAME=couchbase-lite-c-ios-${VERSION}-${BLD_NUM}-enterprise.zip
# curl -O http://latestbuilds.service.couchbase.com/builds/latestbuilds/couchbase-lite-c/${VERSION}/${BLD_NUM}/${ZIP_FILENAME}
# unzip ${ZIP_FILENAME}
# rm ${ZIP_FILENAME}
# popd

pushd $SCRIPT_DIR/../ios/TestServer
mkdir -p cmake
cd cmake
cmake ../../../shared
cd ..
xcodebuild -scheme TestServer -sdk iphonesimulator -configuration Release -derivedDataPath $BUILD_SIM_FOLDER | xcpretty
xcodebuild -scheme TestServer -sdk iphoneos -configuration Release -derivedDataPath $BUILD_DEVICE_FOLDER -allowProvisioningUpdates | xcpretty
popd

pushd $ZIPS_DIR
mv $SCRIPT_DIR/../ios/TestServer/$BUILD_SIM_FOLDER/Build/Products/Release-iphonesimulator/TestServer.app .
mv $SCRIPT_DIR/../ios/TestServer/$BUILD_DEVICE_FOLDER/Build/Products/Release-iphoneos/TestServer.app TestServer-Device.app
zip -ry CBLTestServer-iOS.zip *.app