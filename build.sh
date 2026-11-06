#!/usr/bin/env bash
#
# Dedicated build script for Zephyr TM4C123 project
#

set -euo pipefail

# Project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
NC='\033[0m' # No Color

info()  { echo -e "${BLUE}${BOLD}[INFO]${NC} $*"; }
ok()    { echo -e "${GREEN}${BOLD}[OK]${NC} $*"; }
warn()  { echo -e "${YELLOW}${BOLD}[WARN]${NC} $*"; }
error() { echo -e "${RED}${BOLD}[ERROR]${NC} $*" >&2; }

# Default configuration
DEFAULT_BOARD="tm4c123gxl"
BOARD="${BOARD:-$DEFAULT_BOARD}"
ZEPHYR_PROJECT="${ZEPHYR_PROJECT:-$HOME/zephyrproject}"
VENV_PATH="${ZEPHYR_PROJECT}/.venv"
ZEPHYR_SDK_INSTALL_DIR="${ZEPHYR_SDK_INSTALL_DIR:-$HOME/zephyr-sdk-1.0.1}"
ZEPHYR_BASE="${ZEPHYR_BASE:-$ZEPHYR_PROJECT/zephyr}"

# Action flags
ACTION="build"
PRISTINE=""
FLASH_AFTER_BUILD=false
EXTRA_ARGS=()

usage() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS] [-- <extra west args>]

Dedicated build script for Zephyr TM4C123 project.

Options:
  -b, --board <board>   Target board (default: ${DEFAULT_BOARD})
  -p, --pristine        Force a pristine build (re-runs CMake configuration)
  -f, --flash           Flash the image to board after a successful build
      --flash-only      Flash the existing build binary directly without building
  -c, --clean           Remove build directory and exit
  -m, --menuconfig      Open interactive terminal menuconfig
  -g, --guiconfig       Open interactive GUI configuration
  -h, --help            Show this help message and exit

Examples:
  ./build.sh                     # Normal incremental build
  ./build.sh -p                  # Clean pristine build
  ./build.sh -f                  # Build and flash to board
  ./build.sh --flash-only        # Flash existing firmware to board
  ./build.sh -m                  # Configure Kconfig options via menuconfig
  ./build.sh -p -- -DCONF_FILE=prj.conf
EOF
    exit 0
}

# Parse command line options
while [[ $# -gt 0 ]]; do
    case "$1" in
        -b|--board)
            BOARD="$2"
            shift 2
            ;;
        -p|--pristine)
            PRISTINE="-p always"
            shift
            ;;
        -f|--flash)
            FLASH_AFTER_BUILD=true
            shift
            ;;
        --flash-only)
            ACTION="flash_only"
            shift
            ;;
        -c|--clean)
            ACTION="clean"
            shift
            ;;
        -m|--menuconfig)
            ACTION="menuconfig"
            shift
            ;;
        -g|--guiconfig)
            ACTION="guiconfig"
            shift
            ;;
        -h|--help)
            usage
            ;;
        --)
            shift
            EXTRA_ARGS=("$@")
            break
            ;;
        *)
            warn "Unknown argument '$1' passed to west build"
            EXTRA_ARGS+=("$1")
            shift
            ;;
    esac
done

# Clean action
if [ "$ACTION" = "clean" ]; then
    info "Cleaning build artifacts in ${PROJECT_ROOT}/build ..."
    rm -rf "${PROJECT_ROOT}/build"
    ok "Build directory cleaned."
    exit 0
fi

# 1. Activate Python virtual environment
if [ -d "$VENV_PATH" ]; then
    # shellcheck disable=SC1091
    source "$VENV_PATH/bin/activate"
else
    error "Python virtual environment not found at $VENV_PATH"
    exit 1
fi

# 2. Export Zephyr SDK and Base variables
export ZEPHYR_BASE
export ZEPHYR_SDK_INSTALL_DIR

if [ ! -d "$ZEPHYR_BASE" ]; then
    error "ZEPHYR_BASE directory not found at $ZEPHYR_BASE"
    exit 1
fi

if [ ! -d "$ZEPHYR_SDK_INSTALL_DIR" ]; then
    error "ZEPHYR_SDK_INSTALL_DIR not found at $ZEPHYR_SDK_INSTALL_DIR"
    exit 1
fi

# Handle flash-only action
if [ "$ACTION" = "flash_only" ]; then
    info "Flashing target board: ${BOARD} using west flash..."
    west flash
    ok "Flashing completed."
    exit 0
fi

# Handle menuconfig / guiconfig actions
if [ "$ACTION" = "menuconfig" ]; then
    info "Opening menuconfig for board ${BOARD}..."
    west build -t menuconfig
    exit 0
fi

if [ "$ACTION" = "guiconfig" ]; then
    info "Opening guiconfig for board ${BOARD}..."
    west build -t guiconfig
    exit 0
fi

# Execute build
info "Building Zephyr application"
echo "  Board:       ${BOARD}"
echo "  Zephyr Base: ${ZEPHYR_BASE}"
echo "  Zephyr SDK:  ${ZEPHYR_SDK_INSTALL_DIR}"
if [ -n "$PRISTINE" ]; then
    echo "  Mode:        Pristine (reconfigure)"
else
    echo "  Mode:        Incremental"
fi
echo ""

# Build command execution
BUILD_CMD=(west build -b "$BOARD")
if [ -n "$PRISTINE" ]; then
    BUILD_CMD+=($PRISTINE)
fi
if [ ${#EXTRA_ARGS[@]} -gt 0 ]; then
    BUILD_CMD+=("${EXTRA_ARGS[@]}")
fi

"${BUILD_CMD[@]}"

# Maintain compile_commands.json symlink for IntelliSense / LSP
if [ -f "${PROJECT_ROOT}/build/compile_commands.json" ]; then
    ln -sf "build/compile_commands.json" "${PROJECT_ROOT}/compile_commands.json"
fi

ok "Build successful!"

# Flash if requested
if [ "$FLASH_AFTER_BUILD" = true ]; then
    echo ""
    info "Flashing binary to board ${BOARD}..."
    west flash
    ok "Flashing successful!"
fi
