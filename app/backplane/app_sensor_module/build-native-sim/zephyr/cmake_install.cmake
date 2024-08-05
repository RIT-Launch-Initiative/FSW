# Install script for directory: /home/aaron/Development/Launch/zephyr

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/zephyr/arch/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/zephyr/lib/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/zephyr/soc/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/zephyr/boards/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/zephyr/subsys/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/zephyr/drivers/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/modules/FSW/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/modules/cmsis/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/modules/hal_st/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/modules/stm32/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/modules/littlefs/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/modules/loramac-node/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/zephyr/kernel/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/zephyr/cmake/flash/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/zephyr/cmake/usage/cmake_install.cmake")
  include("/home/aaron/Development/Launch/FSW/app/backplane/app_sensor_module/build-native-sim/zephyr/cmake/reports/cmake_install.cmake")

endif()

