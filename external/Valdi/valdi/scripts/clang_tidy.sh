#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
ROOT_DIR="$SCRIPT_DIR/"
BAZEL_TARGET="@valdi//valdi:omposer:valdi_android"

cd $SCRIPT_DIR/../..
bzl build --config=clang_tidy --bzl_arch=native --keep_going --platforms=@snap_platforms//os:android_arm32 $BAZEL_TARGET
