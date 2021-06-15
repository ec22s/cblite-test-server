#!/bin/bash -ex

# Global define
VERSION=${1}
BLD_NUM=${2}

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
DOWNLOAD_DIR=$SCRIPT_DIR/../downloaded
ZIPS_DIR=$SCRIPT_DIR/../zips

rm -rf $DOWNLOAD_DIR 2> /dev/null
mkdir -p $DOWNLOAD_DIR

pushd $DOWNLOAD_DIR
ZIP_FILENAME=couchbase-lite-c-android-${VERSION}-${BLD_NUM}-enterprise.zip
curl -O http://latestbuilds.service.couchbase.com/builds/latestbuilds/couchbase-lite-c/${VERSION}/${BLD_NUM}/${ZIP_FILENAME}
unzip ${ZIP_FILENAME}
rm ${ZIP_FILENAME}
popd

pushd $SCRIPT_DIR/..
echo "sdk.dir=${ANDROID_HOME}" > local.properties
echo "cmake.prefixPath=${DOWNLOAD_DIR}" >> local.properties
./gradlew assembleRelease