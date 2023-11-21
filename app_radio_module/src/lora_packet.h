/// @file lora_packet.h
/// @brief Contain the definition of the LoRaPacket class.
/// @authors
/// @Naquino14 Nate Aquino naquino14@outlook.com

#ifndef LORA_PACKET_H
#define LORA_PACKET_H

#ifndef ETHERNET_DEFS_H
#include "ethernet_defs.h"
#endif  // !ETHERNET

#ifndef _SYS__STDINT_H
#include <stdint.h>
#endif  // !_SYS__STDINT_H

/// @brief A struct that represents a LoRa packet.
struct lora_packet {
    /// @brief The module that sent the packet.
    ethernet_port module;
    /// @brief The data of the packet.
    uint8_t data[256];
    /// @brief The length of the data.
    uint8_t data_len;
};

#endif  // !LORA_PACKET_H