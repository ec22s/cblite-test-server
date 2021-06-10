#!/bin/bash -ex

# Global define
VERSION=${1}
BLD_NUM=${2}

case "${OSTYPE}" in
    darwin*)  OS="macosx"
              LIBZIP="libzip.5.dylib"
              LIBCBL="libCouchbaseLiteC.dylib"
              ;;
    linux*)   OS="linux"
              LIBZIP="libzip.so.5"
              LIBCBL="libCouchbaseLiteC.so"
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

ZIP_FILENAME=couchbase-lite-c-${OS}-${VERSION}-${BLD_NUM}-enterprise.zip
curl -O http://latestbuilds.service.couchbase.com/builds/latestbuilds/couchbase-lite-c/${VERSION}/${BLD_NUM}/${ZIP_FILENAME}
unzip ${ZIP_FILENAME}
rm ${ZIP_FILENAME}

popd
mkdir -p $BUILD_DIR
pushd $BUILD_DIR

cmake -DCMAKE_PREFIX_PATH=$DOWNLOAD_DIR -DCMAKE_BUILD_TYPE=Release ..
make -j8 install
cp $DOWNLOAD_DIR/lib/$LIBCBL out/bin/
cp out/lib/$LIBZIP out/bin

ZIP_FILENAME=testserver_${OS}_x64.zip
pushd out/bin
zip ${ZIP_FILENAME} *
mkdir -p $ZIPS_DIR
mv ${ZIP_FILENAME} $ZIPS_DIR
