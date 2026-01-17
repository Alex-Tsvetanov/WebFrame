#!/usr/bin/env bash

set -e
set -x

echo "Updating Valdi companion..."

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)" 
BASE_PATH="${SCRIPT_DIR}/../"
PROJECT_ROOT_PATH="${SCRIPT_DIR}/../../../"

skip_analytics=false
companion_output_path=""

usage() {
  echo "Usage: $0 [-o companion_output_path] [-s] [companion_output_path]"
  exit 1
}

# Parse simple command line options
while getopts ":o:s" opt; do
  case "$opt" in
    s)
      skip_analytics=true
      ;;
    o)
      companion_output_path=$OPTARG
      ;;
    \? )
      echo "Invalid option: $OPTARG" 1>&2
      usage
      ;;
    : )
      echo "Invalid option: $OPTARG requires an argument" 1>&2
      usage
      ;;
  esac
done

shift $((OPTIND -1))

if [[ ! -z $ENV_SKIP_ANALYTICS ]] && [ "$ENV_SKIP_ANALYTICS" = true ]; then
    skip_analytics=true
fi

# Assign positional arguments if options were not provided
if [ -z "$companion_output_path" ] && [ $# -ge 1 ]; then
  companion_output_path=$1
  shift
fi

# Main
pushd "$BASE_PATH"

git rev-parse HEAD > ./COMMIT_HASH

rm -rf dist
npm install
npm run test
popd

pushd "$PROJECT_ROOT_PATH"

bzl build //compiler/companion:bundle

real_companion_path=$(realpath bazel-bin/compiler/companion/dist/)

popd



# Move companion to output path if specified
if [ ! -z "$companion_output_path" ] && [ ! $real_companion_path -ef $companion_output_path ]; then
  echo "Companion output path is: $companion_output_path"
  rm -rf "$companion_output_path"
  mkdir -p "$companion_output_path"
  cp -rf $real_companion_path/* "$companion_output_path"
fi

if $skip_analytics; then
    echo "Analytic uploading for this run will be skipped..."
    exit 0
fi
