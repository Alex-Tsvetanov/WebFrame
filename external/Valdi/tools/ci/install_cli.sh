#!/usr/bin/env bash

set -e
set -x

# Intended to be run from open_source/

CLI_DIR="$(cd "./npm_modules/cli"; pwd)"

pushd "$CLI_DIR"

npm run cli:install

# Make sure this completes successfully
valdi --help

popd