cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

include(${ZEPHYR_FSW_MODULE_DIR}/cmake/Autocoders.cmake)


project(airbrake_flight_software LANGUAGES CXX)

target_compile_options(app PRIVATE -Wall -Wno-unused-parameter -Wno-unused-function -Wno-ignored-qualifiers)
FILE(GLOB app_sources src/*.cpp src/*.c)
target_sources(app PRIVATE ${app_sources})

AutocodeTypes(${CMAKE_SOURCE_DIR}/ac/types.yaml)
AutocodeNetworkDefinitions()
add_dependencies(app ac_types)




# Options for LUT Generator
set(LUT_SCRIPT_PATH ${CMAKE_SOURCE_DIR}/tools/lut_to_c.py)
set(LUT_DEFS_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/lut_generated/quantile_lut_data.h)
set(LOWER_PATH ${CMAKE_SOURCE_DIR}/data/quantile_luts/lower_bounds.csv)
set(UPPER_PATH ${CMAKE_SOURCE_DIR}/data/quantile_luts/lower_bounds.csv)


# LUT Generator implementation
get_filename_component(LUT_DEFS_OUTPUT_DIR ${LUT_DEFS_OUTPUT_PATH}
    DIRECTORY BASE_DIR ${APPLICATION_SOURCE_DIR})
make_directory(${LUT_DEFS_OUTPUT_DIR})

message("LUT DEFS " ${LUT_DEFS_OUTPUT_DIR})
message("LUT DEF2 " ${LUT_DEFS_OUTPUT_PATH})

add_custom_command(
    OUTPUT ${LUT_DEFS_OUTPUT_PATH} ${LUT_DEFS_OUTPUT_DIR}
    COMMAND ${PYTHON_EXECUTABLE} ${LUT_SCRIPT_PATH} ${LOWER_PATH} ${UPPER_PATH} > ${LUT_DEFS_OUTPUT_PATH}
    DEPENDS ${LOWER_PATH} ${UPPER_PATH} ${LUT_SCRIPT_PATH}
    VERBATIM
    COMMENT "Generating Quantile LUT C Defines"
)

target_include_directories(app PRIVATE ${LUT_DEFS_OUTPUT_DIR})
target_sources(app PRIVATE ${LUT_DEFS_OUTPUT_PATH})
