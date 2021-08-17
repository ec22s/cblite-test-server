#!/bin/bash -ex

# Global define
VERSION=${1}
BLD_NUM=${2}
EDITION=${3}

case "${OSTYPE}" in
    darwin*)  OS="macosx"
              LIBCBL="libcblite*.dylib"
              ZIP_CMD="unzip"
              ZIP_EXT="zip"
              ;;
    linux*)   LIBCBL="**/libcblite.so*"
              ZIP_CMD="tar xvf"
              ZIP_EXT="tar.gz"
              OS_NAME=`lsb_release -is`
              OS_VERSION=`lsb_release -rs`
              OS_ARCH=`uname -m`
              OS=${OS_NAME,,}${OS_VERSION}_${OS_ARCH}
              ;;
    *)        echo "unknown: $OSTYPE"
              exit 1;;
esac

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
DOWNLOAD_DIR=$SCRIPT_DIR/../downloaded
BUILD_DIR=$SCRIPT_DIR/../build
ZIPS_DIR=$SCRIPT_DIR/../zips

rm -rf $DOWNLOAD_DIR 2> /dev/null
mkdir -p $DOWNLOAD_DIR
pushd $DOWNLOAD_DIR

ZIP_FILENAME=couchbase-lite-c-${EDITION}-${VERSION}-${BLD_NUM}-${OS}.${ZIP_EXT}
curl -O http://latestbuilds.service.couchbase.com/builds/latestbuilds/couchbase-lite-c/${VERSION}/${BLD_NUM}/${ZIP_FILENAME}
${ZIP_CMD} ${ZIP_FILENAME}
rm ${ZIP_FILENAME}

popd
mkdir -p $BUILD_DIR
pushd $BUILD_DIR

cmake -DCMAKE_PREFIX_PATH=$DOWNLOAD_DIR/libcblite-$VERSION -DCMAKE_BUILD_TYPE=Release ..
make -j8 install
cp $DOWNLOAD_DIR/libcblite-$VERSION/lib/$LIBCBL out/bin/

ZIP_FILENAME=testserver_${OS}_${EDITION}.zip
cp $SCRIPT_DIR/../../CBLTestServer-Dotnet/TestServer/sg_cert.pem out/bin
cp -R $SCRIPT_DIR/../../CBLTestServer-Dotnet/TestServer.NetCore/certs out/bin
cp -R $SCRIPT_DIR/../../CBLTestServer-Dotnet/TestServer.NetCore/Databases out/bin
cp -R $SCRIPT_DIR/../../CBLTestServer-Dotnet/TestServer.NetCore/Files out/bin
pushd out/bin
zip -r --symlinks ${ZIP_FILENAME} *
mkdir -p $ZIPS_DIR
mv ${ZIP_FILENAME} $ZIPS_DIR
