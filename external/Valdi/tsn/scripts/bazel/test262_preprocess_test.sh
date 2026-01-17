#!/bin/bash

set -e -o pipefail

usage() { echo -e "Usage: $0 [-b binary_path] [-t test_archive] [-o output_json]" 1>&2; exit 1; }

while getopts "b:t:o:" o; do
    case "${o}" in
        b)
            BINARY=${OPTARG}
            ;;
        t)
            TEST_ARCHIVE=${OPTARG}
            ;;
        o)
            OUTPUT_FILE=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

if [ -z "${BINARY}" ] || [ -z "${TEST_ARCHIVE}" ]|| [ -z "${OUTPUT_FILE}" ]; then
    usage
fi

BINARY_FULL_PATH=$(PWD)/$BINARY
TEST_ARCHIVE_FULL_PATH=$(PWD)/$TEST_ARCHIVE
TEST262_DIR="test262"
WORKING_DIR=$(mktemp -d)

pushd $WORKING_DIR
unzip $TEST_ARCHIVE_FULL_PATH -d $TEST262_DIR &>/dev/null
popd


$BINARY_FULL_PATH -o $(PWD)/$OUTPUT_FILE -t $WORKING_DIR/$TEST262_DIR