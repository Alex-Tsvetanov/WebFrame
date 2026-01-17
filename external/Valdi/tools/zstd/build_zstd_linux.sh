#!/bin/bash

set -x
set -euo pipefail

DIR=$(cd $(dirname "${BASH_SOURCE[0]}"); pwd -P)
pushd "$DIR"

OUTPUT_PATH="$DIR/bin/linux"

rm -rf "${OUTPUT_PATH}"
mkdir -p "${OUTPUT_PATH}"

docker build -t zstd_build .

docker run --rm -v "${OUTPUT_PATH}:/output" zstd_build
