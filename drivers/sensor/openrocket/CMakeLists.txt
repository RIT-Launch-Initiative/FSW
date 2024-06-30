#
#  Copyright (c) 2024 Launch Initiative
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(openrocket_sensors.c)

add_subdirectory_ifdef(CONFIG_OPENROCKET_IMU imu)
# https://stackoverflow.com/questions/49018868/how-do-i-make-cmake-run-a-python-script-before-building-in-order-to-generate-fi