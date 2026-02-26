# Copyright (c) 2025 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
add_subdirectory(frame_handlers)
FILE(GLOB sources *.cpp application/*.cpp)
zephyr_library_sources(${sources})

