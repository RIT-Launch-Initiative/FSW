set(BOARD radio_module)
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(radio-module LANGUAGES C)

FILE(GLOB sources src/*.c)
target_sources(app PRIVATE ${sources})

