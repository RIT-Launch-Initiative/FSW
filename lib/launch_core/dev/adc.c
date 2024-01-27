#include <launch_core/dev/adc.h>

#include <zephyr/device/adc.h>

LOG_MODULE_REGISTER(launch_adc_utils);

int l_init_adc_channel(const struct adc_dt_spec *const channel, struct adc_sequence *const sequence) {
    int ret = adc_is_ready_dt(channel);

    if (ret >= 0) {
        ret = adc_channel_setup_dt(channel);

        if (ret < 0) {
            LOG_ERR("ADC channel %d failed to be setup. Errno %d.", channel->channel_id, ret);
            return ret;
        }

        ret = adc_sequence_init_dt(channel, sequence);
        if (ret < 0) {
            LOG_ERR("ADC channel %d failed to setup sequence. Errno %d", channel->channel_id, ret);
        }

        LOG_INF("ADC channel %d is ready.", channel->channel_id);
    } else {
        LOG_ERR("ADC channel %d is not ready. Errno %d.", channel->channel_id, ret);
    }

    return ret;
}

int l_init_adc_channels(const struct adc_dt_spec *const channels, struct adc_sequence *const sequence,
                        const int num_channels) {
    for (int i = 0; i < num_channels; i++) {
        l_init_adc_channel(&channels[i], &sequence[i]);
    }

    return 0;
}

int l_read_adc_mv(const struct adc_dt_spec *const channel, struct adc_sequence *const sequence, int32_t *val) {
    int ret = adc_read_dt(channel, sequence);
    int32_t val_mv = 0;

    if (ret == 0) {
        if (channel->channel_cfg.differential) { // Differential channels are 16 bits
            val_mv = (int32_t) * ((int16_t *) sequence->buffer);
        } else {
            val_mv = *((int32_t * )(sequence->buffer));
        }

        ret = adc_raw_to_millivolts_dt(channel, &val_mv);
        if (ret < 0) {
            LOG_ERR("Could not convert ADC value from %d to millivolts. Errno %d.", channel->channel_id, ret);
        }

        *val = val_mv;
    } else {
        LOG_ERR("Could not read ADC value from %d. Errno %d.", channel->channel_id, ret);
    }

    return ret;
}

// TODO: Implement
int l_async_read_adc_mv(const struct adc_dt_spec *const channel, struct adc_sequence *const sequence, int32_t *val) {
    return -1;
}