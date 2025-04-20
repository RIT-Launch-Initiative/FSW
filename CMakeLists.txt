# Copyright (c) 2025 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.22)
project(Backplane-Flight-Computer LANGUAGES C CXX)

# Build for compile_commands.json
option(FSW
  "Collect a compile_commands.json over every .c/.cpp in the tree (skips Zephyr build)"
  OFF
)

if (FSW)
  message(STATUS "🔍  FSW=ON → generating full compile_commands.json")

  # Creates compile_commands.json
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  file(GLOB_RECURSE ALL_PROJECT_SRCS
    ${CMAKE_SOURCE_DIR}/app/*.c
    ${CMAKE_SOURCE_DIR}/app/*.cpp
    ${CMAKE_SOURCE_DIR}/lib/f_core/**/*.c
    ${CMAKE_SOURCE_DIR}/lib/f_core/**/*.cpp
  )

  # Object library to register compile rules. Doesn't link so we have multiple 
  # classes, files, functions, etc. with the same name across different projects
  add_library(fsw OBJECT ${ALL_PROJECT_SRCS})

  # pull in any include dirs 
  target_include_directories(fsw PRIVATE
    ${CMAKE_SOURCE_DIR}/include/f_core
  )

  # dip before Zephyr ever sees BOARD
  return()
endif()

# Normal Zephyr build path
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
zephyr_syscall_include_directories(include)
zephyr_include_directories(include)

add_subdirectory(drivers)
add_subdirectory(lib)
