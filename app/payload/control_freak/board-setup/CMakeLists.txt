cmake_minimum_required(VERSION 3.20.0)


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(payload_board_bringup LANGUAGES CXX)

target_compile_options(app PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-ignored-qualifiers)
FILE(GLOB app_sources src/*.cpp)
target_sources(app PRIVATE ${app_sources})
