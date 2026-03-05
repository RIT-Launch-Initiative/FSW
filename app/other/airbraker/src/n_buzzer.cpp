#include <zephyr/drivers/gpio.h>

#define BUZZER_NODE DT_ALIAS(buzzer)
static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(BUZZER_NODE, gpios);

extern "C" int buzzer_init() {

    if (!gpio_is_ready_dt(&buzzer)) {
        // LOG_ERR("Airbrake actuator device enable pin is not ready");
        return -1;
    }
    int ret = gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        // LOG_ERR("Failed to configure airbrakes actuator enable pin");
        return ret;
    }

    return 0;
}

namespace NBuzzer {
static atomic_t alarmSilenced = ATOMIC_INIT(0);
void SilenceAlarm() { atomic_set(&alarmSilenced, ATOMIC_INIT(1)); }
bool ShouldStopAlarm() { return atomic_get(&alarmSilenced) == 1; }

void SetBuzzer(bool buzz) { gpio_pin_set_dt(&buzzer, buzz); }
constexpr uint32_t morse_unit = 50;
void Dot() {
    SetBuzzer(true);
    k_msleep(morse_unit);
    SetBuzzer(false);
}
void Dash() {
    SetBuzzer(true);
    k_msleep(morse_unit * 3);
    SetBuzzer(false);
}
void BetweenLetter() { k_msleep(morse_unit * 3); }
void BetweenWord() { k_msleep(morse_unit * 7); }
void BetweenElement() { k_msleep(morse_unit); }

void MorseBlocking(uint32_t size, const char *str) {
    for (uint32_t i = 0; i < size; i++) {
        switch (str[i]) {
            case '.':
                Dot();
                break;
            case '-':
                Dash();
                break;
            case '/':
                BetweenWord();
                break;
            case ' ':
                BetweenLetter();
                break;
            default:
                break;
        }
        BetweenElement();
    }
}

void NotFlying() {
    while (true) {
        for (int i = 0; i < 10; i++) {
            if (ShouldStopAlarm()) {
                return;
            }

            SetBuzzer(true);
            k_msleep(500);
            SetBuzzer(false);
            k_msleep(250);
        }

        const char help[] = "..-. .-.. .- ... .... / ..-. ..- .-.. .-..";
        NBuzzer::MorseBlocking(sizeof(help), help);

        for (int i = 0; i < 20; i++) {
            if (ShouldStopAlarm()) {
                return;
            }
            SetBuzzer(true);
            k_msleep(500);
            SetBuzzer(false);
            k_msleep(250);
        }
    }
}

void GoodToGoBlocking() {
    const char good[] = "--. --- --- -..";
    NBuzzer::MorseBlocking(sizeof(good), good);
}

} // namespace NBuzzer