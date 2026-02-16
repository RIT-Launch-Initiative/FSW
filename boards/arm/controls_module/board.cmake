# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32F446RE" "--speed=1800" "-verbose")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
