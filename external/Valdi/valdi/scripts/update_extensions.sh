#!/usr/bin/env bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

cd "$SCRIPT_DIR/.."

pushd vscode_debugger
npm install
npm run package
popd

pushd vscode_plugin
npm install
npm run package-extension
popd

cp -rf vscode_debugger/dist/valdi-debug.vsix ../scripts/vscode/valdi-debug.vsix
cp -rf vscode_plugin/build/valdi-vivaldi.vsix ../scripts/vscode/valdi-vivaldi.vsix
