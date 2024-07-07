cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sample_logging LANGUAGES C CXX)

FILE(GLOB app_sources src/*.cpp src/*.c)

target_compile_options(app PRIVATE -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-ignored-qualifiers)
target_sources(app PRIVATE ${app_sources})

