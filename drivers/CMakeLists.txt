# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_SENSOR sensor)
add_subdirectory_ifdef(CONFIG_ADC adc)
add_subdirectory_ifdef(CONFIG_GNSS gnss)
add_subdirectory_ifdef(CONFIG_GORBFS fs/gorbfs)
