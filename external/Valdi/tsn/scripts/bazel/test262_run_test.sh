#!/bin/bash

set -e -o pipefail

usage() { echo -e "Usage: $0 [-b binary_path] [-h harness_archive] [-t testCase.json] [-s skiplist.json]" 1>&2; exit 1; }

while getopts "b:h:t:s:" o; do
    case "${o}" in
        b)
            BINARY=${OPTARG}
            ;;
        h)
            HARNESS_ARCHIVE=${OPTARG}
            ;;
        t)
            TEST_CASES=${OPTARG}
            ;;
        s)
            SKIP_LIST=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

if [ -z "${BINARY}" ] || [ -z "${HARNESS_ARCHIVE}" ] || [ -z "${TEST_CASES}" ] || [ -z "${SKIP_LIST}"]; then
    usage
fi


BINARY_FULL_PATH=$(PWD)/$BINARY
HARNESS_ARCHIVE_FULL_PATH=$(PWD)/$HARNESS_ARCHIVE

BINARY_FULL_PATH=$(PWD)/$BINARY
TEST262_DIR="test262"
TEST262_OUTPUT_FILE="output.txt"

WORKING_DIR=$(mktemp -d)
pushd $WORKING_DIR
unzip $HARNESS_ARCHIVE_FULL_PATH -d $TEST262_DIR &>/dev/null
popd

$BINARY_FULL_PATH -h $WORKING_DIR/$TEST262_DIR -t $TEST_CASES -s $SKIP_LIST