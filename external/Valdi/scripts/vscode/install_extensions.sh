#!/bin/bash

SCRIPT_DIR=$(dirname "$0")

echo
echo "********************************************************************************"
echo "Checking if the 'code' command is installed..."
code --version > /dev/null 2>&1
CODE_AVAILABLE=$?

echo "Checking if the 'cursor' command is installed..."
cursor --version > /dev/null 2>&1
CURSOR_AVAILABLE=$?

if [ $CODE_AVAILABLE -eq 0 ]; then
    echo "'code' command exists!"
fi

if [ $CURSOR_AVAILABLE -eq 0 ]; then
    echo "'cursor' command exists!"
fi

if [ $CODE_AVAILABLE -ne 0 ]; then
    echo
    echo "⚠️ !!!!!!!!!!!! Warning !!!!!!!!!!!!!"
    echo "⚠️ VS Code 'code' command is not installed."
    echo "⚠️ This is required for the standard Valdi development workflow."
    echo "⚠️ "
    echo "⚠️ Please do the following:"
    echo "⚠️ * Launch VS Code"
    echo "⚠️ * Open the Command Palette (either F1 or cmd+shift+P)"
    echo "⚠️ * Type 'shell command' and select > Install 'code' command in PATH"
    echo "⚠️ * Restart the terminal for the new PATH to take effect"
    echo "⚠️ * Run open_source/scripts/vscode/install_extensions.sh (or just scripts/dev_setup.sh)"
    echo "⚠️ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo
fi

if [ $CURSOR_AVAILABLE -ne 0 ]; then
    echo
    echo "⚠️ !!!!!!!!!!!! Warning !!!!!!!!!!!!!"
    echo "⚠️ Cursor 'cursor' command is not installed."
    echo "⚠️ "
    echo "⚠️ If you want to use cursor, please do the following:"
    echo "⚠️ * Launch Cursor"
    echo "⚠️ * Open the Command Palette (either F1 or cmd+shift+P)"
    echo "⚠️ * Type 'shell command' and select > Install 'cursor' command in PATH"
    echo "⚠️ * Restart the terminal for the new PATH to take effect"
    echo "⚠️ * Run open_source/scripts/vscode/install_extensions.sh (or just scripts/dev_setup.sh)"
    echo "⚠️ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo
fi

if [ $CODE_AVAILABLE -ne 0 ] && [ $CURSOR_AVAILABLE -ne 0 ]; then
    exit 1
fi

if [ $CODE_AVAILABLE -eq 0 ]; then
    echo
    echo "********************************************************************************"
    echo "Installing VS Code extensions..."
    echo
    echo "Installing valdi-vivaldi..."
    code --install-extension "$SCRIPT_DIR"/valdi-vivaldi.vsix
    echo
    echo "Installing valdi-debug..."
    code --install-extension "$SCRIPT_DIR"/valdi-debug.vsix
    echo
    echo "Installing Prettier JS..."
    code --install-extension "esbenp.prettier-vscode"
    echo
    echo "Installing Vetur"
    code --install-extension "octref.vetur"
    echo
    echo "Installing ESLint"
    code --install-extension "dbaeumer.vscode-eslint"
fi

if [ $CURSOR_AVAILABLE -eq 0 ]; then
    echo
    echo "********************************************************************************"
    echo "Installing Cursor extensions..."
    echo
    echo "Installing valdi-vivaldi..."
    cursor --install-extension "$SCRIPT_DIR"/valdi-vivaldi.vsix
    echo
    echo "Installing valdi-debug..."
    cursor --install-extension "$SCRIPT_DIR"/valdi-debug.vsix
    echo
    echo "Installing Prettier JS..."
    cursor --install-extension "esbenp.prettier-vscode"
    echo
    echo "Installing Vetur"
    cursor --install-extension "octref.vetur"
    echo
    echo "Installing ESLint"
    cursor --install-extension "dbaeumer.vscode-eslint"
fi
