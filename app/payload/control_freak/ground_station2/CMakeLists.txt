cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

include(${ZEPHYR_FSW_MODULE_DIR}/cmake/Autocoders.cmake)


project(payload_flight_software LANGUAGES CXX)

target_compile_options(app PRIVATE -Wall -Wno-unused-parameter -Wno-unused-function -Wno-ignored-qualifiers)
FILE(GLOB app_sources src/*.cpp src/*.c)
target_sources(app PRIVATE ${app_sources})

AutocodeTypes(${CMAKE_SOURCE_DIR}/ac/types.yaml)
AutocodeNetworkDefinitions()
add_dependencies(app ac_types)

set(LORAMAC_SRC $ENV{ZEPHYR_BASE}/../modules/lib/loramac-node/src)
target_include_directories(app PRIVATE ${LORAMAC_SRC}/radio/ ${LORAMAC_SRC}/system/ ${LORAMAC_SRC}/boards/)
