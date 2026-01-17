#!/usr/bin/env bash

set -e
set -u
set -x

(

# Intended to be run from open_source/
cd "$(dirname "${BASH_SOURCE[0]}")/../.."

# High level core targets
bzl build //valdi:valdi
bzl build //valdi_core:valdi_core

# Dummy libs
bzl build //libs/dummy:dummy
bzl build //libs/dummy:dummy_android

if [[ $(uname) != Linux ]] ; then
    # Hello world apps
    bzl build //apps/helloworld:hello_world_ios
    bzl build //apps/helloworld:hello_world_macos

    # Android depencencies have issues in ci, needs to be fixed
    # bzl build //apps/helloworld:hello_world_android
fi

)
