#ifndef POWER_MODULE_UDP_TRANSMIT_H
#define POWER_MODULE_UDP_TRANSMIT_H

#include <stddef.h>
#include <stdint.h>

struct zbus_channel;
void transmitRawData(const struct zbus_channel* chan);

void transmitDownlinkData(const struct zbus_channel* chan);

#endif //POWER_MODULE_UDP_TRANSMIT_H