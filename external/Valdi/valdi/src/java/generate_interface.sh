#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
cd $SCRIPT_DIR

OUT_FILE=../android/NativeBridge.h

javah -o $OUT_FILE com.snapchat.client.valdi.NativeBridge
