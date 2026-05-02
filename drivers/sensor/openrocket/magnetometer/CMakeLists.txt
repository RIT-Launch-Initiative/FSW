#
#  Copyright (c) 2024 Launch Initiative
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
zephyr_library_sources(openrocket_magnetometer.c)
