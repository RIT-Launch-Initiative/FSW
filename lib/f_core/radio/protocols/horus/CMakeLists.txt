# Copyright (c) 2025 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_HORUSV2)
zephyr_library()
zephyr_library_sources(horus.c golay.c)

endif()
