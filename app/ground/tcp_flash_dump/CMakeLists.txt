#-------------------------------------------------------------------------------
# TCP Flash Dump Application
#
# Copyright (c) 2025 Launch Initiative
# SPDX-License-Identifier: Apache-2.0

set(FSW_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
include(${FSW_ROOT}/cmake/Autocoders.cmake)
include(${FSW_ROOT}/cmake/Snippets.cmake)

AddCommonSnippets()
AddDebugSnippets()

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tcp-flash-dump LANGUAGES C CXX)

FILE(GLOB SOURCES src/*.c src/*.cpp)
target_include_directories(app PRIVATE include)
target_sources(app PRIVATE ${SOURCES})