# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# This CMake file is picked by the Zephyr build system because it is defined
# as the module CMake entry point (see zephyr/module.yml).
cmake_minimum_required(VERSION 3.22)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(Backplane-Flight-Computer)

zephyr_syscall_include_directories(include)

zephyr_include_directories(include)
        
add_subdirectory(drivers)
add_subdirectory(lib)
