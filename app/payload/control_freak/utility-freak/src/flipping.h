int init_flip_hw();

int do_flipping_and_pumping(const struct device *imu_dev, const struct device *barom_dev,
                            const struct device *ina_servo, const struct device *ina_pump);
