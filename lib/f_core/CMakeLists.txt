# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

FILE(GLOB sources
        net/device/*.cpp
        net/network/*.cpp
        net/transport/*.cpp

        device/sensor/*.cpp

        messaging/*.cpp
        
        os/*.cpp

        utils/*.cpp
)
zephyr_library_sources(${sources})
