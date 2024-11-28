
cmake_minimum_required(VERSION 3.13.1)
project(sensor-module LANGUAGES C CXX)

include(../../../cmake/ZephyrProject.cmake)
BootstrapLaunchZephyrProject(core.conf)


FILE(GLOB SOURCES src/*.c src/*.cpp)
target_include_directories(app PRIVATE include)
target_sources(app PRIVATE ${SOURCES})


