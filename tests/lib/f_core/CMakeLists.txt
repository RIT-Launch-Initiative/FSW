# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(f_core_tests)

FILE(GLOB app_sources src/os/*.cpp)
target_sources(app PRIVATE ${app_sources})
