# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
FILE(GLOB sources dev/*.c net/*.c os/*.c utils/*.c *.c)
zephyr_library_sources(${sources})
