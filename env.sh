#!/usr/bin/env bash
# Source this file to set up your environment:
# source env.sh

export ZEPHYR_BASE="$HOME/zephyrproject/zephyr"
export ZEPHYR_SDK_INSTALL_DIR="$HOME/zephyr-sdk-1.0.1"
export BOARD="tm4c123gxl"

# Activate the Zephyr python virtual environment
if [ -f "$HOME/zephyrproject/.venv/bin/activate" ]; then
    source "$HOME/zephyrproject/.venv/bin/activate"
fi

echo "Zephyr environment loaded:"
echo "  ZEPHYR_BASE:            $ZEPHYR_BASE"
echo "  ZEPHYR_SDK_INSTALL_DIR: $ZEPHYR_SDK_INSTALL_DIR"
echo "  Target Board:           $BOARD"
