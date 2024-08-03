#include "f_core/os/c_datalogger.h"
// #include <vecto/r>
#include <zephyr/kernel.h>

struct PacketA {
    uint8_t a;
    uint8_t b;
};
CDataLogger<PacketA> alogger{"/lfs/a.bin"};

int main() {
    printk("asdsadsaads\n");
    for (uint8_t i = 0; i < 100; i++) {
        alogger.write({i, 100 - i});
        k_msleep(10);
    }
    alogger.close();
    return 1;
}