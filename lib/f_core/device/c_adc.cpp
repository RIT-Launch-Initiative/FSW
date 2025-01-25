
#include <f_core/device/c_adc.h>
#include <zephyr/logging/log.h>

// Defensive coding; should not happen
#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

LOG_MODULE_REGISTER(CAdc);

CAdc::CAdc(const adc_dt_spec channel) : channel(channel) {
    if (adc_channel_setup_dt(&channel) < 0) {
        LOG_ERR("Could not set up ADC channel");
        k_panic();
    }
}

int CAdc::UpdateAdcValue() {
    if (!adc_is_ready_dt(&channel)) {
        LOG_ERR("ADC controller device %s not ready\n", channel.dev->name);
        return -1;
    }

    if (adc_read_dt(&channel, &sequence) < 0) {
        LOG_ERR("Could not read from ADC channel");
        return -1;
    }

    value = buff;
    return 0;
}

int32_t CAdc::GetAdcValue() const {
    return value;
}