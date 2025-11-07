#ifndef C_DUMB_FLASH_TENANT_H
#define C_DUMB_FLASH_TENANT_H
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_runnable_tenant.h>
#include <f_core/os/c_tenant.h>
#include <f_core/utils/c_soft_timer.h>
#include <n_autocoder_types.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

class CDumbFlashTenant : public CRunnableTenant {
  public:
    static constexpr size_t ERASE_SIZE = 4096;
    static constexpr size_t ELEMENT_SIZE = sizeof(NTypes::TimestampedSensorData);
    using Element = NTypes::TimestampedSensorData;

    CDumbFlashTenant(const char *name, const struct device *device, size_t flash_offset, size_t partition_size,
                     CMessagePort<NTypes::TimestampedSensorData> &port);

    void Startup() override;
    void Run();

  private:
    const struct device *device;
    size_t flash_offset;
    size_t offset_into_partition = 0;
    size_t partition_size;
    CMessagePort<NTypes::TimestampedSensorData> &port;
};

#endif