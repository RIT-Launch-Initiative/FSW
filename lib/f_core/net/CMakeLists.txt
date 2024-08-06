# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
FILE(GLOB sources device/*.cpp network/*.cpp transport/*.cpp)
zephyr_library_sources(${sources})
