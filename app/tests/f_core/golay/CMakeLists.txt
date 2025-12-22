# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(golay_test)

# Include f_core headers
include_directories(${ZEPHYR_BASE}/../FSW.git/include)

# Add the golay.c source file to the test
target_sources(testbinary
  PRIVATE
  main.c
  ${ZEPHYR_BASE}/../FSW.git/lib/f_core/radio/protocols/horus/golay.c
)
