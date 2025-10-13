#include "f_core/os/c_fs_datalogger.h"

#include <zephyr/kernel.h>

struct Packet {
    uint8_t a;
    uint8_t b;
};
CFsDataLogger<Packet> expand_logger{"/lfs/expand.bin"};
CFsDataLogger<Packet> fill_logger{"/lfs/fill.bin", LogMode::FixedSize, 10};
CFsDataLogger<Packet> wrap_logger{"/lfs/wrap.bin", LogMode::Circular, 10};

int main() {
    for (uint8_t i = 0; i < 100; i++) {
        expand_logger.Write({i, (uint8_t) (100 - i)});
        fill_logger.Write({i, (uint8_t) (100 - i)});
        wrap_logger.Write({i, (uint8_t) (100 - i)});
        k_msleep(10);
    }
    expand_logger.Close();
    fill_logger.Close();
    wrap_logger.Close();
    printk("Finished!\n");
    return 0;
}
