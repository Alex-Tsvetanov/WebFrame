#!/bin/bash

# These tests do not work, they need to be updated

set -e 

kill_child_processes() {
	echo "Killing Child Processes"
	pkill -P $$
}

trap kill_child_processes SIGINT ERR

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd $SCRIPT_DIR

valdi hotreload android --target //apps/helloworld:hello_world_hotreload &

pushd ../vscode_debugger_test
npm install 
npx vscode-dts dev
npm run test
popd

kill_child_processes
