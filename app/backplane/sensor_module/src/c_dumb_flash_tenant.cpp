#include "c_dumb_flash_tenant.h"

LOG_MODULE_REGISTER(dumb_flash);

CDumbFlashTenant::CDumbFlashTenant(const char *name, const struct device *device, size_t flash_offset,
                                   size_t partition_size, CMessagePort<NTypes::TimestampedSensorData> &port)
    : CRunnableTenant(name), device(device), flash_offset(flash_offset), partition_size(partition_size), port(port) {
    if (flash_offset % 4096 != 0) {
        LOG_ERR("Flash offset must be multiple of erase size");
        k_oops();
    }
}

void CDumbFlashTenant::Startup() {}
void CDumbFlashTenant::Run() {
    Element message{};
    while (1) {
        int ret = port.Receive(message, K_FOREVER);
        if (ret != 0) {
            LOG_ERR("Failed to recv from port: %d", ret);
            continue;
        }
        if (offset_into_partition == 0) {
            // erase first page
            flash_erase(device, flash_offset, ERASE_SIZE);
        }
        if (offset_into_partition >= partition_size) {
            LOG_ERR("Saturated partition stopping reading");
            break;
        }
        // from last run, we can assume enough room for us to write is available and erased
        ret = flash_write(device, flash_offset + offset_into_partition, &message, ELEMENT_SIZE);
        if (ret != 0) {
            LOG_WRN("Failed to write. still moving header this block will be bad: %d", ret);
        }

        offset_into_partition += ELEMENT_SIZE;
        size_t offset_in_sector = offset_into_partition % ERASE_SIZE;
        size_t left_in_sector = ERASE_SIZE - offset_in_sector;
        if (left_in_sector <= ELEMENT_SIZE) {
            // erase next sector
            size_t next_page_addr = left_in_sector + offset_into_partition;
            if (next_page_addr % ERASE_SIZE != 0) {
                LOG_ERR("Programmer math error in calculating next sector");
            }
            ret = flash_erase(device, flash_offset + next_page_addr, ERASE_SIZE);
            if (ret != 0) {
                LOG_ERR("Couldnt erase next sector. Your next sector will be nonsense");
            }
        }
    }
}
