cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(mcp3561_sample LANGUAGES C)

FILE(GLOB sources src/*.c)
target_sources(app PRIVATE ${sources})
