set(BOARD sensor_module)
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sensor-module LANGUAGES C)

FILE(GLOB sources src/*.c)
target_sources(app PRIVATE ${sources})

# root include directory
zephyr_include_directories(../include)