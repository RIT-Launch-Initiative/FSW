#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(main);

#define LED_NODE DT_ALIAS(led)
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

// Data of ADC io-channels specified in devicetree.
static const struct adc_dt_spec adc_chan0 =
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

int main() {
  // Setup buffers for reading
  int err;
  uint32_t buf;
  struct adc_sequence sequence = {
      .buffer = &buf,
      /* buffer size in bytes, not number of samples */
      .buffer_size = sizeof(buf),
  };

  // Configure channel prior to sampling.
  if (!adc_is_ready_dt(&adc_chan0)) {
    printk("ADC controller device %s not ready\n", adc_chan0.dev->name);
    return 0;
  }

  err = adc_channel_setup_dt(&adc_chan0);
  if (err < 0) {
    printk("Could not setup channel (%d)\n", err);
    return 0;
  }

  // Measure and print tab separated values
  printk("raw hex, raw dec, volts, volts\n");
  while (true) {
    sequence.channels = adc_chan0.channel_id;

    err = adc_sequence_init_dt(&adc_chan0, &sequence);
    if (err < 0) {
      printk("Could not init seq: %d", err);
    }
    err = adc_read_dt(&adc_chan0, &sequence);
    if (err < 0) {
      printk("Could not read adc channel %d (%d)\n", adc_chan0.channel_id, err);
      continue;
    }

    int32_t val = (int32_t)buf;
    float volts = 2.4f * ((float)val) / ((float)0x7fffff);

    printk("0x%06x,\t%8d,\t%2.4f V\t\r", buf, val, (double)volts);

    gpio_pin_toggle_dt(&led);

    k_msleep(100);
  }
}
