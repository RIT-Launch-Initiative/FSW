#-------------------------------------------------------------------------------
# Backplane Power Module Application
#
# Copyright (c) 2023 Aaron Chan
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(power-module LANGUAGES C)

FILE(GLOB sources src/*.c)
target_sources(app PRIVATE ${sources})
