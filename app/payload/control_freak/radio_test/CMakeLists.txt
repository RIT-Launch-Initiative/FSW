cmake_minimum_required(VERSION 3.20.0)


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(payload_board_bringup LANGUAGES CXX)


set(LORAMAC_SRC $ENV{ZEPHYR_BASE}/../modules/lib/loramac-node/src)


target_compile_options(app PRIVATE -Wall -Wextra -Wno-unused-function -Wno-ignored-qualifiers -Wno-missing-field-initializers)
FILE(GLOB app_sources src/*.cpp)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE ${LORAMAC_SRC}/radio/ ${LORAMAC_SRC}/system/ ${LORAMAC_SRC}/boards/)
