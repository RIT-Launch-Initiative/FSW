# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

FILE(GLOB sources
        net/device/*.cpp
        net/network/*.cpp
        net/transport/*.cpp

        device/*.cpp
        device/sensor/*.cpp
        flight/*.cpp
        messaging/*.cpp
        
        os/*.cpp

        utils/*.cpp
)

add_subdirectory(protocols/horus)

zephyr_library_sources(${sources})
