# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
# TODO: Separate the sources into their respective directories
FILE(GLOB sources
        net/network/*.cpp
        net/transport/*.cpp
        os/*.cpp)
zephyr_library_sources(${sources})
