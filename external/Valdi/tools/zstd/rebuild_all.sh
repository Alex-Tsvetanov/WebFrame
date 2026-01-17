#!/bin/bash

set -x
set -euo pipefail

DIR=$(cd $(dirname "${BASH_SOURCE[0]}"); pwd -P)

$DIR/build_zstd_macos.sh
$DIR/build_zstd_linux.sh
