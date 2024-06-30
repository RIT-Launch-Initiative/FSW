#
#  Copyright (c) 2024 Launch Initiative
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()


add_subdirectory_ifdef(CONFIG_OPENROCKET_IMU imu)
# https://stackoverflow.com/questions/49018868/how-do-i-make-cmake-run-a-python-script-before-building-in-order-to-generate-fi


get_filename_component(CANONICAL_OPENROCKET_DATA_PATH "${CONFIG_OPENROCKET_DATA_FILE}"
                       REALPATH BASE_DIR ${APPLICATION_SOURCE_DIR})

if (${CONFIG_OPENROCKET_IMU})
  set(IMU_FLAG "-i")
endif()
                       
if (${CONFIG_OPENROCKET_BAROMETER})
  set(BAROM_FLAG "-b")
endif()

if (${CONFIG_OPENROCKET_GNSS})
  set(BAROM_FLAG "-g")
endif()

add_custom_command(
 OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/openrocket_data.c
 COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/conversion.py ${IMU_FLAG} ${BAROM_FLAG} ${GNSSFLAG} ${CANONICAL_OPENROCKET_DATA_PATH} ${CMAKE_CURRENT_BINARY_DIR}/openrocket_data.c
 DEPENDS ${CANONICAL_OPENROCKET_DATA_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/conversion.py
 VERBATIM
 COMMENT "Generating Openrocket Data Library"
)

zephyr_include_directories(.)
zephyr_library_sources(openrocket_sensors.c ${CMAKE_CURRENT_BINARY_DIR}/openrocket_data.c)

# add_dependencies(enum_test run)