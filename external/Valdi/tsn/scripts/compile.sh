#!/bin/bash

set -e -o pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
PROTOTYPE_DIR="$( realpath $SCRIPT_DIR/.. )"
COMPANION_DIR="$PROTOTYPE_DIR/../companion"
OUTPUT_DIR="$PROTOTYPE_DIR/output"
IGNORED_FILES="[]"

if [[ "$1" == "examples" ]]; then
    STRIP_PREFIX=$(pwd)/input/examples
    INPUT_FOLDER="$PROTOTYPE_DIR/input/examples/"
    OUTPUT_FOLDER="$OUTPUT_DIR/examples"
    IS_TEST=false
elif [[ "$1" == "test262" ]]; then
    TEST_REPO=$(readlink -f "$SCRIPT_DIR/../../../../third-party/test262/src/")
    INPUT_FOLDER="$TEST_REPO/test262/test/"
    STRIP_PREFIX="$TEST_REPO"

    OUTPUT_FOLDER="$OUTPUT_DIR/test262"
    IS_TEST=true
    IGNORED_FILES=`awk 'BEGIN {first=1; printf "["} {if (first) first=0; else printf ","; printf "\"%s\"", $0} END {printf "] "}' $SCRIPT_DIR/ignored_test262.txt`
else
    echo -e "Usage: $0 examples or $0 test262" 1>&2; exit 1;
fi

cd $COMPANION_DIR
rm -rf $OUTPUT_FOLDER
scripts/run_dev.sh --log-to-stderr --command "compileNative" --body "{\"inputFolder\": \"$INPUT_FOLDER\", \"outputFolder\": \"$OUTPUT_FOLDER\", \"stripPrefix\": \"$STRIP_PREFIX\", \"isTest262\" : $IS_TEST, \"ignoredFiles\": $IGNORED_FILES}"