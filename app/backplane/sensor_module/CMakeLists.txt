#-------------------------------------------------------------------------------
# Backplane sensor Module Application
#
# Copyright (c) 2023 Launch Initiative
# SPDX-License-Identifier: Apache-2.0

set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Guarantee that prj.conf is the default and first configuration file to be read

list(APPEND CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/core.conf)

if (DEBUG)
    message(STATUS "Adding debug configuration")
    list(APPEND CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/debug.conf)
endif ()

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sensor-module LANGUAGES C CXX)

FILE(GLOB SOURCES src/*.c src/*.cpp)
target_include_directories(app PRIVATE include)
target_sources(app PRIVATE ${SOURCES})
#list(APPEND CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/prj.conf)



