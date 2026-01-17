#!/usr/bin/env bash

# Fail on errors
set -e

CURRENT_SYSTEM=`uname`

if [ "$CURRENT_SYSTEM" == "Darwin" ] && [ "$(arch)" == "i386" ] && [ "$(sysctl sysctl.proc_translated)" == "sysctl.proc_translated: 1" ];
then
    MSG="terminal is currently running under rosetta and dev_setup.sh does not support installation in this mode"
    PREFIX=""
    HELPFUL=$'IMPORTANT:\n\t1 - With Terminal closed (Important) Go to finder -> /Applications/Utilities/\n\t2 - Right Click on Terminal and select "Get Info"\n\t3 - On the info panel uncheck "Open using Rosetta"'
    if [ $TERM_PROGRAM == "vscode" ]
    then
        PREFIX="VSCode "
        HELPFUL=$'IMPORTANT:\n\tRerun the script outside of VSCode using the system terminal'
    fi
    echo "$PREFIX$MSG"
    echo "$HELPFUL"
    exit 1
fi