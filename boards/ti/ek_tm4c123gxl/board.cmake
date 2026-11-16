# SPDX-License-Identifier: Apache-2.0

# TI Tiva C Series TM4C123GXL LaunchPad board configuration

board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd.cfg")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
