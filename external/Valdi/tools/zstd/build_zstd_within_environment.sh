#!/bin/bash

set -x
set -euo pipefail

BUILD_LOCATION=${1:?Build location not set (first arg)}
OUTPUT_LOCATION=${2:?Output location not set (second arg)}

rm -rf $BUILD_LOCATION
mkdir -p $BUILD_LOCATION

pushd "$BUILD_LOCATION"
git clone https://github.com/facebook/zstd.git --depth=1 --no-single-branch
cd zstd

git checkout tags/v1.4.5 -b v1.4.5-branch

cmake build/cmake
make zstd

cp ./programs/zstd "$OUTPUT_LOCATION"
