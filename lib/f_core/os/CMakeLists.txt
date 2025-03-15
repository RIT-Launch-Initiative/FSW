# Copyright (c) 2025 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_F_CORE_BASE64_DATA_DUMP_SHELL base64dump)

zephyr_library()
FILE(GLOB sources *.cpp)
zephyr_library_sources(${sources})

