# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()


add_subdirectory_ifdef(CONFIG_F_CORE_OS os)
add_subdirectory_ifdef(CONFIG_F_CORE_NET net)
add_subdirectory_ifdef(CONFIG_F_CORE_NET net)

FILE(GLOB sources
        net/device/*.cpp
        net/network/*.cpp
        net/transport/*.cpp
        
        device/sensor/*.cpp

        messaging/*.cpp
        
        os/*.cpp)
zephyr_library_sources(${sources})
