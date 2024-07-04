#
#  Copyright (c) 2024 Launch Initiative
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()


add_subdirectory_ifdef(CONFIG_OPENROCKET_IMU imu)
add_subdirectory_ifdef(CONFIG_OPENROCKET_BAROMETER barometer)
add_subdirectory_ifdef(CONFIG_OPENROCKET_GNSS gnss)


get_filename_component(CANONICAL_OPENROCKET_DATA_PATH "${CONFIG_OPENROCKET_DATA_FILE}"
                       REALPATH BASE_DIR ${APPLICATION_SOURCE_DIR})

if (${CONFIG_OPENROCKET_IMU})
  set(IMU_FLAG "-i")
endif()
                       
if (${CONFIG_OPENROCKET_BAROMETER})
  set(BAROM_FLAG "-b")
endif()

if (${CONFIG_OPENROCKET_GNSS})
  set(GNSS_FLAG "-g")
endif()

add_custom_command(
 OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/openrocket_data.c
 COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/conversion.py ${IMU_FLAG} ${BAROM_FLAG} ${GNSS_FLAG} ${CANONICAL_OPENROCKET_DATA_PATH} ${CMAKE_CURRENT_BINARY_DIR}/openrocket_data.c
 DEPENDS ${CANONICAL_OPENROCKET_DATA_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/conversion.py
 VERBATIM
 COMMENT "Generating Openrocket Data Library"
)

zephyr_include_directories(.)
zephyr_library_sources(openrocket_sensors.c ${CMAKE_CURRENT_BINARY_DIR}/openrocket_data.c)
