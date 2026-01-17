#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

pushd "$SCRIPT_DIR"

./mac_deps_setup.sh

echo
echo "********************************************************************************"
echo "Will install ios-webkit-debug-proxy"
brew install ios-webkit-debug-proxy

popd
