#!/bin/bash

set -e -o pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
PROTOTYPE_DIR="$( realpath $SCRIPT_DIR/.. )"

usage() { echo "Usage: $0 [-s <strip prefix>] [-b <bin_dir>] [-c <compiler_command>] [-o <output_file.c>]" 1>&2; exit 1; }

BIN_DIR=""
STRIP_INCLUDE_PREFIX=""
COMPILER_CMD=""
INPUT_FILES=()
OUTPUT_FILE=""

while [ $OPTIND -le "$#" ]
do
    if getopts s:b:c:o: option
    then
        case $option
        in
        s)
            STRIP_INCLUDE_PREFIX=${OPTARG}
            ;;
        b)
            BIN_DIR=${OPTARG}
            ;;
        c)
            COMPILER_CMD=${OPTARG}
            ;;
        o)
            OUTPUT_FILE="$PWD/${OPTARG}"
            ;;
        esac
    else
        INPUT_FILES+=("$PWD/${!OPTIND}")
        ((OPTIND++))
    fi
done

if [[ -z "$INPUT_FILES" ||  -z "$STRIP_INCLUDE_PREFIX"  || -z "$OUTPUT_FILE" ]] ; then
    usage
fi


FORMATTED_INPUT_FILES=""
# Loop through the array
for element in "${INPUT_FILES[@]}"; do
  if [[ -z "$FORMATTED_INPUT_FILES" ]]; then
    FORMATTED_INPUT_FILES="\"${element}\""
  else
    FORMATTED_INPUT_FILES="$FORMATTED_INPUT_FILES, \"${element}\""
  fi
done

export BAZEL_BINDIR="$BIN_DIR"
$COMPILER_CMD --log-to-stderr --command "compileNative" --body "{\"registerInputFiles\": true, \"stripIncludePrefix\": \"$STRIP_INCLUDE_PREFIX\", \"outputFile\": \"$OUTPUT_FILE\", \"inputFiles\": [$FORMATTED_INPUT_FILES]}"
