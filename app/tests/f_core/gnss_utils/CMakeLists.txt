# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(gnss_utils_test)

# Include f_core headers
# Try to find FSW root from Zephyr base or use current project structure
if(EXISTS "${ZEPHYR_BASE}/../FSW.git")
    set(FSW_ROOT "${ZEPHYR_BASE}/../FSW.git")
elseif(EXISTS "${ZEPHYR_BASE}/../FSW")
    set(FSW_ROOT "${ZEPHYR_BASE}/../FSW")
else()
    # Fallback to relative path from test directory
    set(FSW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
endif()

include_directories(${FSW_ROOT}/include)

target_sources(testbinary
  PRIVATE
  main.c
)
