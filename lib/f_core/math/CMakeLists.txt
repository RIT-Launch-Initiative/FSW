# Copyright (c) 2025 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
FILE(GLOB sources *.cpp)
zephyr_library_sources(${sources})

