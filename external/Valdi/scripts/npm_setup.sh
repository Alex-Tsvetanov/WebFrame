#!/usr/bin/env bash

# Fail on errors
set -e

CURRENT_SYSTEM=`uname`

if [[ $CURRENT_SYSTEM == "Linux" ]]
then
    curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash
    export NVM_DIR="$([ -z "${XDG_CONFIG_HOME-}" ] && printf %s "${HOME}/.nvm" || printf %s "${XDG_CONFIG_HOME}/nvm")"
else
    brew install nvm
    export NVM_DIR="$HOME/.nvm"
fi

# Load NVM
[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"

nvm install v22
nvm use v22
nvm alias default v22

npm install -g npm@8
