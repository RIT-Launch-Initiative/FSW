#include "f_core/radio/protocols/horus/horus.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/types.h>
#define FXOSC_HZ                       32000000
#define RFM_FSTEP_HZ                   61.03515625
#define RFM_MAX_FREQUENCY_DEVIATION_HZ 999879

// BS ALERT
#include "sx1276/sx1276.h"

#define RADIORST_NODE DT_ALIAS(radioreset)
static const struct gpio_dt_spec radioreset = GPIO_DT_SPEC_GET(RADIORST_NODE, gpios);

#define MAX_SATS 20
extern struct gnss_data last_data;
extern int current_sats;
extern int current_tracked_sats;
extern struct gnss_satellite last_sats[MAX_SATS];
extern uint64_t last_fix_uptime;

extern bool is_transmitting;
extern bool is_horus;
extern struct k_timer radio_timer;

const char noradio_prompt[] = "(X)uart:~$";
const char horus_prompt[] = "(H)uart:~$";

// Horus Data

uint16_t horus_seq_number = 0;

int32_t set_pramble_len(uint16_t preamble_len) {
    uint8_t msb = (preamble_len >> 8) & 0xff;
    uint8_t lsb = (preamble_len >> 8) & 0xff;
    SX1276Write(REG_PREAMBLEMSB, msb);
    SX1276Write(REG_PREAMBLELSB, lsb);
    return 0;
}

/**
 * Calculate the values needed for Fdev registers from a frequency in Hertz
 * @return -ERANGE if the frequency is out of range. 0 if its in range
 */
static int32_t frequency_dev_regs_from_fdev(uint32_t fdev, uint8_t *msb, uint8_t *lsb) {
    if (fdev > 200000) {
        return -ERANGE;
    }
    // Fdev = Fstep * Fdev[15,0]
    // Fdev / Fstep = Fdev[15,0]
    // Fstep = FX_OSC / 2^19
    // (Fdev * 2^19) / FX_OSC = Fdev[15,0]
    uint16_t val = (uint16_t) ((double) fdev / RFM_FSTEP_HZ);
    *msb = (val >> 8) & 0xff;
    *lsb = val & 0xff;
    return 0;
}

static int32_t set_frequency_deviation_by_reg(uint8_t msb, uint8_t lsb) {
    uint8_t data[2] = {msb, lsb};
    SX1276WriteBuffer(REG_FDEVMSB, data, 2);
    return 0;
}
static int32_t set_frequency_deviation(uint32_t freq_dev) {
    uint8_t msb = 0;
    uint8_t lsb = 0;
    if (frequency_dev_regs_from_fdev(freq_dev, &msb, &lsb) < 0) {
        return -ERANGE;
    }

    return set_frequency_deviation_by_reg(msb, lsb);
}

/**
 * Sets the RF carrier frequency registers on the module.
 * This does not immediately change the frequency that the module is transmitting at or receiving from.
 * The in use RF Carrier frequency is only changed when:
 * - Entering FSRX/FSTX modes
 * - Restarting the receiver 
 * Datasheet pg. 89
 */
int32_t set_frequency_by_reg(uint8_t msb, uint8_t mid, uint8_t lsb) {
    uint8_t reg_frf[3] = {msb, mid, lsb};
    SX1276WriteBuffer(REG_FRFMSB, reg_frf, 3);
    return 0;
}

int32_t frf_reg_from_frequency(uint32_t freq, uint8_t *msb, uint8_t *mid, uint8_t *lsb) {

    ////Frf = Fstep x Frf(23,0)
    // Frf(23,0) = Frf / Fstep
    const double fstep = 61.03515625;
    uint32_t frf = freq / fstep;
    *msb = (frf >> 16) & 0xff;
    *mid = (frf >> 8) & 0xff;
    *lsb = (frf) & 0xff;

    return 0;
}

/**
 * Sets the RF carrier frequency.
 * This does not immediately change the frequency that the module is transmitting at or receiving from.
 * The in use RF Carrier frequency is only changed when:
 * - Entering FSRX/FSTX modes
 * - Restarting the receiver 
 * Datasheet pg. 89
 * @param dev the radio device
 * @param freq the frequency to set to
 * @return -ERANGE if the requested frequency is out of range for this model
 * 0 if the frequency was succesfully set
 * other sub 0 error codes from spi interaction errors.
 */
int32_t set_carrier_frequency(uint32_t freq) {
    uint8_t msb = 0;
    uint8_t mid = 0;
    uint8_t lsb = 0;
    int32_t res = frf_reg_from_frequency(freq, &msb, &mid, &lsb);
    if (res < 0) {
        return res;
    }
    res = set_frequency_by_reg(msb, mid, lsb);
    if (res < 0) {
        return res;
    }
    return res;
}

#define DT_DRV_COMPAT semtech_sx1276

struct sx127x_config {
    struct spi_dt_spec bus;
    struct gpio_dt_spec reset;
#if DT_INST_NODE_HAS_PROP(0, antenna_enable_gpios)
    struct gpio_dt_spec antenna_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, rfi_enable_gpios)
    struct gpio_dt_spec rfi_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios)
    struct gpio_dt_spec rfo_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
    struct gpio_dt_spec pa_boost_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, tcxo_power_gpios)
    struct gpio_dt_spec tcxo_power;
#endif
};

/**
 * Perform a 'Manual Reset' of the module 
 * (Datasheet pg.111 section 7.2.2)
 * @param config the module config 
 * @return any errors from configuring GPIO
 */
int32_t rfm9xw_software_reset() {
    printk("Software resetting radio with %s pin %d\n", radioreset.port->name, (int) radioreset.pin);

    if (gpio_pin_configure_dt(&radioreset, GPIO_OUTPUT | GPIO_PULL_DOWN) < 0) {
        printk("Failed to set pin to 0 to reset chip\n");
    }

    k_usleep(150); // >100us

    if (gpio_pin_configure_dt(&radioreset, GPIO_DISCONNECTED) < 0) {
        printk("Failed to set pin to 0 to reset chip\n");
    }
    k_msleep(5);

    return 0;
}
void transmit_horus(uint8_t *buf, int len) {
    printk("Transmitting horus packet len %d\n", len);
    const uint32_t carrier = 434000000;
    const float deviation = 405;

    const uint32_t bitrate = 100;
    const int usec_per_symbol = 10079; //1000000 / bitrate;

    const uint32_t high = carrier + deviation;
    const uint32_t step = (uint32_t) ((float) deviation * 2.f / 3.f);
    const uint32_t symbols_fdev[4] = {3 * step, 2 * step, step, 0};

    SX1276Write(REG_OPMODE,
                RF_OPMODE_LONGRANGEMODE_OFF | RF_OPMODE_MODULATIONTYPE_FSK | RF_OPMODE_SLEEP); // Standby FSK

    SX1276Write(REG_OPMODE,
                RF_OPMODE_LONGRANGEMODE_OFF | RF_OPMODE_MODULATIONTYPE_FSK | RF_OPMODE_STANDBY); // Standby FSK
    SX1276Write(REG_PLLHOP, RF_PLLHOP_FASTHOP_ON | 0x2d); // Fast hop on | default value
    SX1276Write(REG_PACONFIG, 0b11111111);
    set_pramble_len(0);
    set_carrier_frequency(high);

    // start transmitting
    set_frequency_deviation(symbols_fdev[3]);
    SX1276Write(REG_OPMODE, RF_OPMODE_LONGRANGEMODE_OFF | RF_OPMODE_MODULATIONTYPE_FSK | RF_OPMODE_TRANSMITTER);

    uint64_t startms = k_uptime_get();
    struct k_timer bitrate_timer;
    k_timer_init(&bitrate_timer, NULL, NULL);
    k_timer_start(&bitrate_timer, K_USEC(usec_per_symbol), K_USEC(usec_per_symbol));
    int preamble_len = 16;
    // transmit preamble 0,1,2,3 (low, 2nd lowest, 2nfd highest, highest)
    for (int i = 0; i < preamble_len; i++) {
        for (int j = 0; j < 4; j++) {
            k_timer_status_sync(&bitrate_timer);
            set_frequency_deviation(symbols_fdev[3 - j]);
        }
    }

    for (int byte_index = 0; byte_index < len; byte_index++) {
        const uint8_t byte = buf[byte_index];
        const uint8_t syms[4] = {
            (uint8_t) ((byte >> 6) & 0b11),
            (uint8_t) ((byte >> 4) & 0b11),
            (uint8_t) ((byte >> 2) & 0b11),
            (uint8_t) ((byte >> 0) & 0b11),
        };
        for (int sym_index = 0; sym_index < 4; sym_index++) {
            uint8_t sym = syms[sym_index];
            uint32_t fdev = symbols_fdev[sym];
            k_timer_status_sync(&bitrate_timer);
            set_frequency_deviation(fdev);
        }
    }
    // Last symbol
    k_timer_status_sync(&bitrate_timer);
    uint64_t endms = k_uptime_get();
    printf("Elapsed: %d ms\n", (int) (endms - startms));
    // Then turn off
    SX1276Write(REG_OPMODE,
                RF_OPMODE_LONGRANGEMODE_OFF | RF_OPMODE_MODULATIONTYPE_FSK | RF_OPMODE_STANDBY); // Standby FSK
    k_timer_stop(&bitrate_timer);

    return;
}

void make_and_transmit_horus() {
    double lat = (double) last_data.nav_data.latitude / 180e9f;
    double lon = (double) last_data.nav_data.longitude / 180e9f;
    uint16_t alt = last_data.nav_data.altitude;
    struct horus_packet_v2 data{
        .payload_id = 808,
        .counter = horus_seq_number,
        .hours = last_data.utc.hour,
        .minutes = last_data.utc.minute,
        .seconds = (uint8_t) (last_data.utc.millisecond / 1000),
        .latitude = (float) lat,
        .longitude = (float) lon,
        .altitude = alt,
        .speed = last_data.nav_data.speed,
        .sats = (uint8_t) current_sats,
        .temp = 40,
        .battery_voltage = 255,
        .custom_data = {1, 2, 3, 4, 5, 6, 7, 8, 9},
        .checksum = 0,
    };

    horus_seq_number++;
    horus_packet_v2_encoded_buffer_t packet = {0};
    horusv2_encode(&data, &packet);

    transmit_horus(&packet[0], sizeof(packet));
}

int cmd_horustx(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    if (is_transmitting) {
        shell_prompt_change(shell, noradio_prompt);
        shell_print(shell, "Stop Transmitting");
        k_timer_stop(&radio_timer);
        is_transmitting = false;
    } else {
        shell_prompt_change(shell, horus_prompt);
        shell_print(shell, "Sending Horus");
        is_horus = true;
        is_transmitting = true;
        k_timer_start(&radio_timer, K_MSEC(5000), K_MSEC(5000));
    }

    return 0;
}