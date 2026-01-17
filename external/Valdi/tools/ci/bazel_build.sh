#!/usr/bin/env bash

set -e
set -x

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
ROOT_DIR="$(cd "$SCRIPT_DIR/../../../../"; pwd)"
OPEN_SOURCE_DIR="$(cd "$SCRIPT_DIR/../../"; pwd)"

pushd "$ROOT_DIR"

# Flags
export CLIENT_FLAVOR=client_development

if [[ $(uname) == Linux ]] ; then
    sudo apt-get update -y
    sudo apt-get install -y openjdk-8-jdk libboost-all-dev
    sudo update-java-alternatives --set java-1.8.0-openjdk-amd64
    export JAVA_HOME=/usr/lib/jvm/java-1.8.0-openjdk-amd64
    export ANDROID_HOME=$HOME/Android/Sdk
else
    export JAVA_HOME=/Library/Java/JavaVirtualMachines/jdk1.8.0_151.jdk/Contents/Home
    export ANDROID_HOME=$HOME/Library/Android/sdk
fi

export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/23.0.7599858

export ANDROID_SDK=$ANDROID_HOME
export ANDROID_SDK_ROOT=$ANDROID_SDK

mkdir -p "$ANDROID_HOME"

./Jenkins/install/install_android_sdk.sh $ANDROID_HOME
./Jenkins/install/install_ndk_r23.sh -d $ANDROID_NDK_HOME -f
ln -s "$ANDROID_HOME/cmdline-tools/latest" "$ANDROID_HOME/tools" || true # Do not error if symlink exists
$ANDROID_HOME/tools/bin/sdkmanager "platforms;android-35"
$ANDROID_HOME/tools/bin/sdkmanager "build-tools;34.0.0"

# Get all of the archives.json deps in the right place
./tools/repo-archiver.sh pull

popd

pushd "$OPEN_SOURCE_DIR"

# Everything we want to make sure builds in CI

./tools/ci/build_core_targets.sh

# Test suite
./tools/ci/run_tests.sh

# Reroute global because we can't sudo anything
mkdir -p ~/.npm-global/lib
npm config set prefix '~/.npm-global'
npm install -g npm@8
PATH=~/.npm-global/bin:$PATH

./scripts/mirroring/git_init.sh

./tools/ci/install_cli.sh

./tools/ci/bootstrap_app.sh

popd
