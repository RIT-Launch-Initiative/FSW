# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
FILE(GLOB sources
        net/device/*.cpp
        device/sensor/*.cpp
        os/*.cpp)
zephyr_library_sources(${sources})
