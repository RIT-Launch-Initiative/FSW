set(BOARD nucleo_f446re)
set(SHIELD launch_mikroe) 
# set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(mcp3561_sample LANGUAGES C)

FILE(GLOB sources src/*.c)
target_sources(app PRIVATE ${sources})
