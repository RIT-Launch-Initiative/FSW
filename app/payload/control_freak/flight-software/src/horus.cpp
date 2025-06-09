#include "f_core/radio/protocols/horus/horus.h"

#include "gps.h"
#include "sx1276/sx1276.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/types.h>
LOG_MODULE_REGISTER(radio);

#define FXOSC_HZ                       32000000
#define RFM_FSTEP_HZ                   61.03515625
#define RFM_MAX_FREQUENCY_DEVIATION_HZ 999879

// BS ALERT
#include "sx1276/sx1276.h"

#define RADIORST_NODE DT_ALIAS(radioreset)
static const struct gpio_dt_spec radioreset = GPIO_DT_SPEC_GET(RADIORST_NODE, gpios);

extern horus_packet_v2 get_telemetry();

static int32_t set_pramble_len(uint16_t preamble_len) {
    uint8_t msb = (preamble_len >> 8) & 0xff;
    uint8_t lsb = (preamble_len >> 8) & 0xff;
    SX1276Write(REG_PREAMBLEMSB, msb);
    SX1276Write(REG_PREAMBLELSB, lsb);
    return 0;
}

static int32_t set_power_config(uint8_t output_power, uint8_t max_power, bool plus20_en) {
    // RegOCP over current protection
    // MaxPower: 3bits = 10.8+0.6*MaxPower
    // default 0x4, max 0b111=0x7
    if (max_power > 7) {
        LOG_WRN("Unsupported max power %d setting default 0x4", (int) max_power);
        max_power = 0x04;
    }
    // Pout = 2+OutputPower if PA BOOST Pin
    // Pout = Pmax - (15 - OutputPower) if RFO Pin
    if (output_power > 15) {
        LOG_WRN("Unsupported max power %d. Setting default 0xf", (int) output_power);
        output_power = 0xf;
    }
    // wired to use PA_BOOST
    uint8_t regpacfg = 0b1000000 | (max_power << 4) | output_power;
    SX1276Write(REG_PACONFIG, regpacfg);

    uint8_t pa_dac_val = 0;
    if (plus20_en) {
        pa_dac_val = 0x47;
    } else {
        pa_dac_val = 0x44;
    }
    LOG_INF("PADAC: %02x", pa_dac_val);
    SX1276Write(REG_PADAC, pa_dac_val);
    return 0;
}
static uint8_t SX1276GetPaSelect(int8_t power) {
    if (power > 14) {
        return RF_PACONFIG_PASELECT_PABOOST;
    } else {
        return RF_PACONFIG_PASELECT_RFO;
    }
}

void SX1276SetRfTxPower(int8_t power) {
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    paConfig = SX1276Read(REG_PACONFIG);
    paDac = SX1276Read(REG_PADAC);

    paConfig = (paConfig & RF_PACONFIG_PASELECT_MASK) | SX1276GetPaSelect(power);

    if ((paConfig & RF_PACONFIG_PASELECT_PABOOST) == RF_PACONFIG_PASELECT_PABOOST) {
        if (power > 17) {
            paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_ON;
        } else {
            paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_OFF;
        }
        if ((paDac & RF_PADAC_20DBM_ON) == RF_PADAC_20DBM_ON) {
            if (power < 5) {
                power = 5;
            }
            if (power > 20) {
                power = 20;
            }
            paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t) ((uint16_t) (power - 5) & 0x0F);
        } else {
            if (power < 2) {
                power = 2;
            }
            if (power > 17) {
                power = 17;
            }
            paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t) ((uint16_t) (power - 2) & 0x0F);
        }
    } else {
        if (power > 0) {
            if (power > 15) {
                power = 15;
            }
            paConfig = (paConfig & RF_PACONFIG_MAX_POWER_MASK & RF_PACONFIG_OUTPUTPOWER_MASK) | (7 << 4) | (power);
        } else {
            if (power < -4) {
                power = -4;
            }
            paConfig = (paConfig & RF_PACONFIG_MAX_POWER_MASK & RF_PACONFIG_OUTPUTPOWER_MASK) | (0 << 4) | (power + 4);
        }
    }
    SX1276Write(REG_PACONFIG, paConfig);
    SX1276Write(REG_PADAC, paDac);
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
static int32_t set_frequency_by_reg(uint8_t msb, uint8_t mid, uint8_t lsb) {
    uint8_t reg_frf[3] = {msb, mid, lsb};
    SX1276WriteBuffer(REG_FRFMSB, reg_frf, 3);
    return 0;
}

static int32_t frf_reg_from_frequency(uint32_t freq, uint8_t *msb, uint8_t *mid, uint8_t *lsb) {

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
static int32_t set_carrier_frequency(uint32_t freq) {
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

/**
 * Perform a 'Manual Reset' of the module 
 * (Datasheet pg.111 section 7.2.2)
 * @param config the module config 
 * @return any errors from configuring GPIO
 */
static int32_t rfm9xw_software_reset() {
    LOG_INF("Software resetting radio with %s pin %d\n", radioreset.port->name, (int) radioreset.pin);

    if (gpio_pin_configure_dt(&radioreset, GPIO_OUTPUT | GPIO_PULL_DOWN) < 0) {
        LOG_WRN("Failed to set pin to 0 to reset chip\n");
    }

    k_usleep(150); // >100us

    if (gpio_pin_configure_dt(&radioreset, GPIO_DISCONNECTED) < 0) {
        LOG_WRN("Failed to set pin to 0 to reset chip\n");
    }
    k_msleep(5);

    return 0;
}

int calc_us_per_fsk_symbol() {
    float skew = get_skew_smart();
    LOG_INF("Skew of %f", skew);
    static constexpr uint32_t bitrate = 100; // symbols per second
    static constexpr uint32_t useconds_per_second = 1000 * 1000;
    static constexpr uint32_t useconds_per_symbol = useconds_per_second / bitrate;
    uint32_t useconds_skew_adjusted = useconds_per_symbol * skew;
    LOG_INF("useconds of %d", useconds_skew_adjusted);
    return useconds_skew_adjusted;
}

static void transmit_horus(uint8_t *buf, int len) {
    const uint32_t carrier = 432950000;
    const float deviation = 405;

    int us_per_symbol = calc_us_per_fsk_symbol();

    const uint32_t high = carrier + deviation;
    const uint32_t step = (uint32_t) ((float) deviation * 2.f / 3.f);
    const uint32_t symbols_fdev[4] = {3 * step, 2 * step, step, 0};

    SX1276Write(REG_OPMODE,
                RF_OPMODE_LONGRANGEMODE_OFF | RF_OPMODE_MODULATIONTYPE_FSK | RF_OPMODE_SLEEP); // Standby FSK

    SX1276Write(REG_OPMODE,
                RF_OPMODE_LONGRANGEMODE_OFF | RF_OPMODE_MODULATIONTYPE_FSK | RF_OPMODE_STANDBY); // Standby FSK
    SX1276Write(REG_PLLHOP, RF_PLLHOP_FASTHOP_ON | 0x2d); // Fast hop on | default value

    SX1276SetRfTxPower(20);
    SX1276Write(REG_OCP, 0b00100000 + 28);

    set_pramble_len(0);
    set_carrier_frequency(high);

    // start transmitting
    set_frequency_deviation(symbols_fdev[3]);
    SX1276Write(REG_OPMODE, RF_OPMODE_LONGRANGEMODE_OFF | RF_OPMODE_MODULATIONTYPE_FSK | RF_OPMODE_TRANSMITTER);

    struct k_timer bitrate_timer;
    k_timer_init(&bitrate_timer, NULL, NULL);

    k_timer_start(&bitrate_timer, K_USEC(us_per_symbol), K_USEC(us_per_symbol));
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
    // Then turn off
    SX1276Write(REG_OPMODE,
                RF_OPMODE_LONGRANGEMODE_OFF | RF_OPMODE_MODULATIONTYPE_FSK | RF_OPMODE_STANDBY); // Standby FSK
    k_timer_stop(&bitrate_timer);

    return;
}

int packet_count = 0;
K_MUTEX_DEFINE(horus_mutex);
uint8_t horus_temp_c = 0;
uint16_t horus_press = 0;
uint8_t horus_battery_v_2 = 0;
small_orientation horus_snorm = {0};
FlightState horus_state = FlightState::NotSet;

int submit_horus_data(const float &tempC, const float &press, const float &batteryVoltage,
                      const small_orientation &snorm, const FlightState &fs) {
    int ret = k_mutex_lock(&horus_mutex, K_MSEC(20));
    if (ret != 0) {
        LOG_WRN("Couldnt lock mut to send horus data");
        return ret;
    }
    horus_temp_c = tempC;
    uint16_t press16 = press * 100;
    horus_press = press16;
    // horus battery is 0-5V 0 -255LSB
    horus_battery_v_2 = (uint8_t) ((batteryVoltage / 10.0f) * 255);

    horus_snorm = snorm;
    horus_state = fs;
    return k_mutex_unlock(&horus_mutex);
}

horus_packet_v2 get_telemetry() {
    horus_packet_v2 pac{};
    packet_count++;

    // pac.payload_id = 808;
    pac.payload_id = 388;
    pac.counter = packet_count;

    int ret = fill_horus_packet_with_gps(&pac);
    if (ret != 0) {
        LOG_WRN("Failed to fill packet with GPS data: %d", ret);
    }

    ret = k_mutex_lock(&horus_mutex, K_MSEC(20));
    if (ret != 0) {
        LOG_WRN("Couldnt lock mut for horus");
        return pac;
    }
    pac.battery_voltage = horus_battery_v_2;
    pac.temp = horus_temp_c;

    pac.custom_data[0] = horus_snorm.x;
    pac.custom_data[1] = horus_snorm.y;
    pac.custom_data[2] = horus_snorm.z;
    pac.custom_data[4] = horus_press & 0xff;
    pac.custom_data[5] = (horus_press >> 8) & 0xff;

    pac.custom_data[7] = (uint8_t) horus_state;

    k_mutex_unlock(&horus_mutex);
    return pac;
}

void make_and_transmit_horus() {
    struct horus_packet_v2 data = get_telemetry();

    horus_packet_v2_encoded_buffer_t packet = {0};
    horusv2_encode(&data, &packet);

    transmit_horus(&packet[0], sizeof(packet));
}

void wait_for_timeslot() {
    uint32_t ms = millis_till_timeslot_opens();
    k_msleep(ms);
}

int radio_thread(void *, void *, void *) {
    if (is_data_locked()) {
        return -1;
    }
    k_msleep(5000);

    SX1276SetRfTxPower(6);

    for (int i = 0; i < 10; i++) {
        LOG_INF("Startup: %d", i);
        make_and_transmit_horus();
        k_msleep(5);
    }
    SX1276SetRfTxPower(20);
    while (true) {
        // Maybe make packet AOT and only transmit at timeslot
        wait_for_timeslot();
        LOG_ERR("TIME");
        // LOG_INF("Transmitting Horus");
        make_and_transmit_horus();
        k_msleep(5000); // chill
    }
    return 0;
}
