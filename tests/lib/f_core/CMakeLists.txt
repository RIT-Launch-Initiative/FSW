cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(f_core_tests)

FILE(GLOB sources src/*.c src/*.cpp)
target_sources(f_core_tests PRIVATE ${sources})
