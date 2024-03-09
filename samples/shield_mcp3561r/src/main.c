#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

#define ADC_NODE DT_NODELABEL(adc)

#if !DT_NODE_EXISTS(ADC_NODE)
#error "something went very wrong. You forgot an ADC"
#endif

const struct device *my_adc = DEVICE_DT_GET(ADC_NODE);

int main() {
  if (my_adc == NULL) {
    LOG_ERR("No mcp3561r ADC Found");
    return -1;
  }
  if (!device_is_ready(my_adc)) {
    printk("\nError: Device \"%s\" is not ready; "
           "check the driver initialization logs for errors.\n",
           my_adc->name);
    return -1;
  }
}