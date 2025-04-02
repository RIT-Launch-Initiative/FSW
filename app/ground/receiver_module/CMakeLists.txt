#-------------------------------------------------------------------------------
# Backplane Radio Module Application
#
# Copyright (c) 2023 Launch Initiative
# SPDX-License-Identifier: Apache-2.0

set(FSW_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
include(${FSW_ROOT}/cmake/Snippets.cmake)
include(${FSW_ROOT}/cmake/Autocoders.cmake)

AddCommonSnippets()

# Guarantee that core.conf is the default and first configuration file to be read
list(APPEND CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/core.conf)

if (DEBUG)
    message(STATUS "Adding debug configuration")
    AddDebugSnippets()
    list(APPEND CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/debug.conf)
endif ()

if (BOARD STREQUAL "native_sim")
    if (UNIX AND NOT APPLE)
        message(STATUS "Adding simulation configuration")
        AddSimSnippets()
        list(APPEND CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/sim.conf)
    else ()
        message(FATAL_ERROR "Simulation is only supported on Linux systems")
    endif ()
endif ()

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(receiver-module LANGUAGES C CXX)

FILE(GLOB SOURCES src/*.c src/*.cpp)
target_include_directories(app PRIVATE include)
target_sources(app PRIVATE ${SOURCES})

AutocodeTypes()
AutocodeNetworkDefinitions()
add_dependencies(app ac_net_defs ac_types)
