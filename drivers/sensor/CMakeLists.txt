# Copyright (c) 2024, RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_ADXL375 adxl375)
add_subdirectory_ifdef(CONFIG_MS56XX ms56xx)
add_subdirectory_ifdef(CONFIG_INA260 ina260)
add_subdirectory_ifdef(CONFIG_OPENROCKET_SENSORS openrocket)
add_subdirectory_ifdef(CONFIG_SIMSHUNT simshunt)

