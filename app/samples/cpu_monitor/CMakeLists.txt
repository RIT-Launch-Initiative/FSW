cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(cpu-monitor LANGUAGES C CXX)

FILE(GLOB sources src/*.c src/*.cpp)
target_sources(app PRIVATE ${sources})
