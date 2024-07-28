#include "f_core/os/datalogger.h"
// #include <vecto/r>
#include <zephyr/kernel.h>

struct PacketA {
    int a;
    int b;
};
CDataLogger<PacketA> alogger{"/lfs/a.bin"};

int main() {
    printk("asdsadsaads\n");
    for (int i = 0; i < 100; i++) {
        alogger.write({i, 100 - i});
        k_msleep(10);
    }
    alogger.close();
    return 1;
}