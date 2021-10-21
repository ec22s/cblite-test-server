#!/bin/bash -ex

# Global define
VERSION=${1}
BLD_NUM=${2}
EDITION=${3}

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
DOWNLOAD_DIR=$SCRIPT_DIR/../downloaded
BUILD_DIR=$SCRIPT_DIR/../build
ZIPS_DIR=$SCRIPT_DIR/../zips

rm -rf $DOWNLOAD_DIR 2> /dev/null
mkdir -p $DOWNLOAD_DIR
pushd $DOWNLOAD_DIR

ZIP_FILENAME=couchbase-lite-c-${EDITION}-${VERSION}-${BLD_NUM}-macos.zip
curl -O http://latestbuilds.service.couchbase.com/builds/latestbuilds/couchbase-lite-c/${VERSION}/${BLD_NUM}/${ZIP_FILENAME}
unzip ${ZIP_FILENAME}
rm ${ZIP_FILENAME}

popd
mkdir -p $BUILD_DIR
pushd $BUILD_DIR

cmake -DCMAKE_PREFIX_PATH=$DOWNLOAD_DIR/libcblite-$VERSION -DCMAKE_BUILD_TYPE=Release -DCBL_MACOS_ARCH=x86_64 ..
make -j8 install
mv out/bin/testserver testserver-x86_64
cmake -DCMAKE_PREFIX_PATH=$DOWNLOAD_DIR/libcblite-$VERSION -DCMAKE_BUILD_TYPE=Release -DCBL_MACOS_ARCH=arm64 ..
make -j8 install
mv out/bin/testserver testserver-arm64
cp $DOWNLOAD_DIR/libcblite-$VERSION/lib/libcblite*.dylib out/bin/
lipo testserver-x86_64 testserver-arm64 -create -output out/bin/testserver

ZIP_FILENAME=testserver_macos_${EDITION}.zip
cp $SCRIPT_DIR/../../CBLTestServer-Dotnet/TestServer/sg_cert.pem out/bin
cp -R $SCRIPT_DIR/../../CBLTestServer-Dotnet/TestServer.NetCore/certs out/bin
cp -R $SCRIPT_DIR/../../CBLTestServer-Dotnet/TestServer.NetCore/Databases out/bin
cp -R $SCRIPT_DIR/../../CBLTestServer-Dotnet/TestServer.NetCore/Files out/bin
pushd out/bin
zip -r --symlinks ${ZIP_FILENAME} *
mkdir -p $ZIPS_DIR
mv ${ZIP_FILENAME} $ZIPS_DIR
