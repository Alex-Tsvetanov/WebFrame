#!/usr/bin/env bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
CLIENT_ROOT="$SCRIPT_DIR/../../"
pushd "$CLIENT_ROOT"

bzl run snap_drawing:benchmark --snap_flavor=production -c opt
