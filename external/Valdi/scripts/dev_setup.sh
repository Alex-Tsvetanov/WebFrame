#!/usr/bin/env bash

# Fail on errors
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

CURRENT_SYSTEM=`uname`

pushd "$SCRIPT_DIR"

./verify_not_rosetta.sh

./config_setup.sh

./npm_setup.sh

if [[ $CURRENT_SYSTEM == "Linux" ]]
then
    ./linux_dev_setup.sh
else
    ./macos_dev_setup.sh
fi

popd

echo
echo
echo "-> All done."

echo "================================================================================================"
echo "Your ~/.bashrc or ~/.zshrc may have been updated, make sure to open a new shell or source it now yourself:"
echo "source ~/.bashrc"
echo "source ~/.zshrc"
echo "================================================================================================"
