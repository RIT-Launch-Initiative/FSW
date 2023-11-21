/// @file ethernet_defs.h
/// @brief Contains the definition of the ethernet_port enum.
/// @authors
/// @Naquino14 Nate Aquino naquino14@outlook.com
#ifndef ETHERNET_DEFS_H
#define ETHERNET_DEFS_H

/// @brief Used to identify the module that sent the packet.
enum ethernet_port {
    /// @brief The sensor mod port.
    sensor_mod = 8000,
    /// @brief The power mod port.
    power_mod = 9000,
    /// @brief The autopilot mod port.
    autopilot_mod = 10000,
};

#endif  // !ETHERNET_DEFS_H
