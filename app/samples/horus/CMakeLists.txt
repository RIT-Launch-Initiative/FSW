# Copyright (c) 2025 Launch Initiative
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app_lib_custom_test)

target_sources(app PRIVATE src/main.c)
