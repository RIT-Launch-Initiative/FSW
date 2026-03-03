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

void SetBuzzer(bool buzz) { gpio_pin_set_dt(&buzzer, buzz); }
constexpr uint32_t morse_unit = 40;
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

void MorseBlocking(uint32_t size, const char *str) {
    for (uint32_t i = 0; i < size; i++) {
        switch (str[i]) {
            case '.':
                Dot();
                break;
            case '-':
                Dash();
                break;
            case ' ':
                BetweenWord();
                break;
        }
        BetweenLetter();
    }
}
void NogoBlocking() {
    const char nogo[] = "-. --- --. ---";
    NBuzzer::MorseBlocking(sizeof(nogo), nogo);
}

    // const char e[] = ". . .";
    // const char sos[] = "... --- ...";
    // const char help[] = ".... . .-.. .--. / ... --- -- . - .... .. -. --. / .... .- ... / --. --- -. . / .-- .-. --- -. --.";
    // NBuzzer::MorseBlocking(sizeof(sos), sos);


} // namespace NBuzzer