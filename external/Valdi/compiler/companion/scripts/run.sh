#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd $SCRIPT_DIR/../dist
node --enable-source-maps --max-old-space-size=8000 bundle.js "${@:1}"
