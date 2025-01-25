#ifndef C_ADC_H
#define C_ADC_H

#include <zephyr/drivers/adc.h>

class CAdc {
public:
    /**
     * Constructor
     * @param[in] channel IO channel to get the value from
     */
    explicit CAdc(const adc_dt_spec channel);

    /**
     * Destructor
     */
    ~CAdc() = default;

    /**
     * Samples ADC for a new value
     * @return 0 on success, -1 on failure
     */
    int UpdateAdcValue();

    /**
     * Get the value from the ADC
     * @return Raw ADC value in millivolts
     */
    int32_t GetAdcValue() const;

private:
    const adc_dt_spec channel;
    int32_t value = 0;
    uint32_t buff = 0;
    struct adc_sequence sequence = {
        .buffer = &buff,
        .buffer_size = sizeof(buff),
    };
};

#endif //C_ADC_H