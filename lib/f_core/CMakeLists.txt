# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

FILE(GLOB sources
        net/device/*.cpp
        net/network/*.cpp
        net/transport/*.cpp
        net/application/*.cpp
        device/sensor/*.cpp

        messaging/*.cpp
        
        os/*.cpp)
zephyr_library_sources(${sources})
