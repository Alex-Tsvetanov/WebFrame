#!/usr/bin/env bash

# Fail on errors
set -e

USER_VALDI_DIR="$HOME/.valdi"
USER_VALDI_CONFIG_PATH="$USER_VALDI_DIR/config.yaml"

echo
echo "********************************************************************************"
echo "Checking Valdi config exists at $USER_VALDI_CONFIG_PATH..."
if [[ -f "$USER_VALDI_CONFIG_PATH" ]]; then
    echo "$USER_VALDI_CONFIG_PATH already exists!"
else
    echo "$USER_VALDI_CONFIG_PATH doesn't exist, creating..."
    mkdir -p "$USER_VALDI_DIR"
    echo "logs_output_dir: ~/.valdi/logs" > "$USER_VALDI_CONFIG_PATH"
fi