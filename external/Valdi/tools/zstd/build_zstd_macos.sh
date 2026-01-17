#!/bin/bash

set -x
set -euo pipefail

if [ "$(uname)" != "Darwin" ]; then
    echo "Tool must be run from MacOS environment (Found '$(uname)')"
    exit 1
fi

DIR=$(cd $(dirname "${BASH_SOURCE[0]}"); pwd -P)
pushd "$DIR"

OUTPUT_PATH="$DIR/bin/macos"

rm -rf "${OUTPUT_PATH}"
mkdir -p "${OUTPUT_PATH}"

SCRATCH_PATH="$(mktemp -d)"
./build_zstd_within_environment.sh "${SCRATCH_PATH}" "${OUTPUT_PATH}"

# best effort cleanliness
rm -rf "${SCRATCH_PATH}"

