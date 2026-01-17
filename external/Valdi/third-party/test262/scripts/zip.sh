#!/bin/bash

set -e -o pipefail

ZIP_PATH=$1
SRC_PATH=$PWD/src/third-party/test262/src/test262
FOLDER=$2
TMP_ZIP_NAME=tmp.zip

pushd $SRC_PATH
zip -r $TMP_ZIP_NAME $FOLDER > /dev/null
popd

mv $SRC_PATH/$TMP_ZIP_NAME $ZIP_PATH