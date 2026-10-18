# Copyright (c) 2025 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
FILE(GLOB sources *.cpp)
zephyr_library_sources(${sources})
zephyr_library_sources_ifdef(CONFIG_F_CORE_CPU_MONITOR tenants/c_cpu_monitor_tenant.cpp)
