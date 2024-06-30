#include <zephyr/kernel.h>

int or_data_interpolator();

int main() {
    while (1) {
        printk("asdadsa %d\n", or_data_interpolator());
        k_msleep(1000);
    }
    return 1;
    // return 0;
}
