cmake_minimum_required(VERSION 3.20.0)

set(BOARD "nucleo_f446re")

find_package(Zephyr)
project(nucleo_tests)
# FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE src/main.c)
