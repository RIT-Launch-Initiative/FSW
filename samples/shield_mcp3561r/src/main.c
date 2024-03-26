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

#define ADC_NODE DT_NODELABEL(mcp3561)

#define LED_NODE DT_ALIAS(led)
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

#define IRQ_NODE DT_NODELABEL(irq)
const struct gpio_dt_spec irq = GPIO_DT_SPEC_GET(IRQ_NODE, gpios);

/* Data of ADC io-channels specified in devicetree. */

static const struct adc_dt_spec adc_chan0 =
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

int init_led();
int main() {

  init_led();

  int err;
  uint32_t count = 0;
  uint32_t buf;
  struct adc_sequence sequence = {
      .buffer = &buf,
      /* buffer size in bytes, not number of samples */
      .buffer_size = sizeof(buf),
  };

  /* Configure channels individually prior to sampling. */
  // for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
  if (!adc_is_ready_dt(&adc_chan0)) {
    printk("ADC controller device %s not ready\n", adc_chan0.dev->name);
    return 0;
  }
  //
  err = adc_channel_setup_dt(&adc_chan0);
  if (err < 0) {
    printk("Could not setup channel (%d)\n", err);
    return 0;
  }
  // }

  while (true) {
    // printk("ADC reading[%u]:\n", count++);
    int32_t val;

    // printk("- %s, channel %d: \n", adc_chan0.dev->name,
    // adc_chan0.channel_id);
    sequence.channels = adc_chan0.channel_id;

    err = adc_sequence_init_dt(&adc_chan0, &sequence);
    if (err < 0) {
      printk("Could not init seq: %d", err);
    }
    err = adc_read_dt(&adc_chan0, &sequence);
    if (err < 0) {
      printk("Could not read (%d)\n", err);
      continue;
    }

    /*
     * If using differential mode, the 16 bit value
     * in the ADC sample buffer should be a signed 2's
     * complement value.
     */
    if (adc_chan0.channel_cfg.differential) {
      val = (int32_t)((int16_t)buf);
    } else {
      val = (int32_t)buf;
    }
    // int vref_mv = 2400;
    float vref = 2.4;
    float resolution = (1 << 24);
    float top = 6667200;
    float bot = 30420;
    float valf = val;
    float vs = ((valf - bot) / (top - bot));
    // (float)val
    // if (adc_raw_to_millivolts_dt(&adc_chan0, &val) != 0) {
    // printk("No conversion\n");
    // }
    printk("%d          \r", val);

    gpio_pin_toggle_dt(&led);

    // printk("LED toggle\n");

    k_msleep(100);
  }
}

int init_led() {
  // Boilerplate: set up GPIO
  if (!gpio_is_ready_dt(&irq)) {
    LOG_ERR("GPIO is not ready\n");
    return -1;
  }
  return 0;
}