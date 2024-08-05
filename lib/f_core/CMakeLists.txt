# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_F_CORE_OS os)
add_subdirectory_ifdef(CONFIG_F_CORE_NET net)
add_subdirectory_ifdef(CONFIG_F_CORE_SENSOR device/sensor)
