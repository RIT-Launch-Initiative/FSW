set(BOARD potato)
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(POTATO LANGUAGES C)

FILE(GLOB sources src/*.c)
target_include_directories(app PRIVATE include)
target_sources(app PRIVATE ${sources})
