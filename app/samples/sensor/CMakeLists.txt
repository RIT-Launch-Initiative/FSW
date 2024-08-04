cmake_minimum_required(VERSION 3.20.0)

#set(BOARD nucleo_f446re)
#set(SHIELD launch_mikroe)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sample_logging LANGUAGES C)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
