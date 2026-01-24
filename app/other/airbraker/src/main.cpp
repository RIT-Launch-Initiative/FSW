#include "servo.h"
#include "n_storage.h"

#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include "matrix.hpp"

LOG_MODULE_REGISTER(main, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

SYS_INIT(servo_init, APPLICATION, 1);
SYS_INIT(storage_init, APPLICATION, 2);

int main() {
    printk("Time to break air\n");
    volatile float x = 0;
    Matrix<2,2> m1 = Matrix<2,2>::Zeros();
    Matrix<2,2> m2 = Matrix<2,2>::Identity();
    m1.vals[0][0] = x;
    auto m3 = m1*m2;
    printk("%f\n", (double)m3.vals[0][0]);
}
