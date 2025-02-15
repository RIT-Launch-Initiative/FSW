# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

add_subdirectory_ifdef(CONFIG_F_CORE_DEVICE device)
add_subdirectory_ifdef(CONFIG_F_CORE_SENSOR device/sensor)
add_subdirectory_ifdef(CONFIG_F_CORE_NET net)
add_subdirectory_ifdef(CONFIG_F_CORE_RADIO radio)
add_subdirectory_ifdef(CONFIG_F_CORE_OS os)
add_subdirectory_ifdef(CONFIG_F_CORE_UTIL utils)

