# Copyright (c) 2024 Aaron Chan
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
FILE(GLOB sources dev/*.c net/*.c os/*.c)
zephyr_library_sources(${sources})
