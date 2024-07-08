# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
FILE(GLOB sources os/*.cpp)
add_subdirectory(net)
zephyr_library_sources(${sources})
