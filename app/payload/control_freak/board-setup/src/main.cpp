#include <zephyr/kernel.h>

int main() {
    while (true) {
        printk("Hello!\n");
        k_msleep(500);
    }
    return 0;
}