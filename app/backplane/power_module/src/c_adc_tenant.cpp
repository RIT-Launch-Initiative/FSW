// This is how you can get the ADC channel from the device tree, only index zero from the zephyr user node since theres only one
// Ask aaron if there will ever be a future ADC channel
// const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

#include <f_core/device/c_adc.h>