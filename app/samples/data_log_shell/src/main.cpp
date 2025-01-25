#include "f_core/os/c_data_log_shell.h"

#include <zephyr/kernel.h>


int main() {
    CDataLogger<uint32_t> timestampLogger{"/lfs/timestamps.bin"};

    for (uint8_t i = 0; i < 100; i++) {
        expand_logger.write({i, (uint8_t) (100 - i)});
        fill_logger.write({i, (uint8_t) (100 - i)});
        wrap_logger.write({i, (uint8_t) (100 - i)});
        k_msleep(10);
    }

    timestampLogger.close();
    printk("Finished!\n");
    return 0;
}