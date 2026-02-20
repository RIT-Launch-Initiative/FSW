#include "n_model.h"
#include "n_storage.h"
#include "servo.h"

#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

SYS_INIT(servo_init, APPLICATION, 1);
SYS_INIT(storage_init, APPLICATION, 2);

int main() {
    printk("Time to break air\n");

    float effort = NModel::CalcActuatorEffort(1039.0475, 270.98915);
    printk("Effort: %f\n", (double)effort);
}
