#include "n_model.h"
#include "n_storage.h"
#include "servo.h"

#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

SYS_INIT(servo_init, APPLICATION, 1);
SYS_INIT(storage_init, APPLICATION, 2);

volatile float alt = 1.0;
volatile float acc = 0;

int main() {
    using namespace NModel;

    k_msleep(1000);

    printk("starting....\n");

    k_msleep(1000);



    printk("Time to break air\n");
    
}
