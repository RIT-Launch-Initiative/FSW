# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
#find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(horus LANGUAGES C CXX)
# This is awful but kconfig enabling f_core does not seem to bring in the root CMakeLists.txt that adds f_core includes
# include_directories(${ZEPHYR_BASE}/../FSW.git/include)

target_sources(testbinary
  PRIVATE
  main.c
)
