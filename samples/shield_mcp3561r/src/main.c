#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
LOG_MODULE_REGISTER(main);

#define ADC_NODE DT_NODELABEL(adc)

const struct device *my_adc = DEVICE_DT_GET(ADC_NODE);

// static const struct adc_channel_cfg ch0_cfg_dt =
// ADC_CHANNEL_CFG_DT(DT_CHILD(ADC_NODE, DT_NODELABEL(chan)));

int main() {
  LOG_INF("Initializing ADC");
  if (!device_is_ready(my_adc)) {
    LOG_ERR("ADC isn't ready");
    return -1;
  }
  // adc_channel_setup(my_adc, &ch0_cfg_dt);
}