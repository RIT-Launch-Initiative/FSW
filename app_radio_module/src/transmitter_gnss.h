#ifndef RADIO_MODULE_TRANSMITTER_GNSS_H
#define RADIO_MODULE_TRANSMITTER_GNSS_H

#include <stdint.h>
#include <zephyr/kernel.h>

void config_gnss_tx_time(k_timeout_t interval);

#endif //RADIO_MODULE_TRANSMITTER_GNSS_H
