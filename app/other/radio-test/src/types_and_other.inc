#include <stdint.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/gnss.h>

static float nanodegreesToDegreesScale = 1e9f;
static float millimeterToMeterScale = 1e3f;

float nanodeg_to_deg(const int64_t latitude) { return (float) (latitude) / nanodegreesToDegreesScale; }

float mm_to_m(const int64_t altitude) { return (float) (altitude) / millimeterToMeterScale; }


enum pac_size {
    PAC_SIZE_SMALL,
    PAC_SIZE_LARGE,
};

const char *bw_to_str(enum lora_signal_bandwidth bw) {
    switch (bw) {
        case BW_125_KHZ:
            return "125 kHz";
        case BW_250_KHZ:
            return "250 kHz";
        case BW_500_KHZ:
            return "500 kHz";
        default:
            return "unknown";
    }
}

struct packet_required {
    float lat;
    float lon;
    float alt;
    struct gnss_time time;
    enum gnss_fix_status fix;
    uint8_t sats;
};