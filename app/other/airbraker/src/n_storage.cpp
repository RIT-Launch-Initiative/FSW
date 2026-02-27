#include "n_storage.hpp"

#include "common.hpp"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(storage);

// 2 pages so we can flip flop and always have one valid page
#define BOOTCOUNT_PARTITION DT_NODE_BY_FIXED_PARTITION_LABEL(bootcount_storage)
// storage for singly-stored parameters such as gyro bias and ground level pressure
#define PARAM_PARTITION DT_NODE_BY_FIXED_PARTITION_LABEL(parameter_storage)
// full of Packets (first N are preboost packets and then flight packets. sorted by timestamp)
#define FLIGHT_PARTITION DT_NODE_BY_FIXED_PARTITION_LABEL(flight_storage)

#ifdef CONFIG_BOARD_NATIVE_SIM
const struct device* flashDevice = DEVICE_DT_GET_ONE(zephyr_sim_flash);
#else
const struct device* flashDevice = DEVICE_DT_GET(DT_GPARENT(FLIGHT_PARTITION));
#endif

// Smallest erasable size
#define SECTOR_SIZE 4096
// Largest erasable size (w/o erasing entire chip)
#define BIG_SECTOR_SIZE (64 * 1024)

constexpr off_t BOOTCOUNT_PARTITION_OFFSET = DT_REG_ADDR(BOOTCOUNT_PARTITION);
constexpr off_t PARAM_PARTITION_OFFSET = DT_REG_ADDR(PARAM_PARTITION);
constexpr off_t FLIGHT_PARTITION_OFFSET = DT_REG_ADDR(FLIGHT_PARTITION);

constexpr off_t BOOTCOUNT_PARTITION_SIZE = DT_REG_SIZE(BOOTCOUNT_PARTITION);
constexpr off_t PARAM_PARTITION_SIZE = DT_REG_SIZE(PARAM_PARTITION);
constexpr off_t FLIGHT_PARTITION_SIZE = DT_REG_SIZE(FLIGHT_PARTITION);

static_assert(BOOTCOUNT_PARTITION_SIZE >= 2 * SECTOR_SIZE && IS_ALIGNED(BOOTCOUNT_PARTITION_OFFSET, SECTOR_SIZE),
              "Invalid place or size for bootcount partition. Needs 4KB Alignment");
static_assert(PARAM_PARTITION_SIZE == SECTOR_SIZE && IS_ALIGNED(PARAM_PARTITION_OFFSET, SECTOR_SIZE),
              "Invalid place or size for parameter partition. Needs 4KB Alignment");
static_assert(IS_ALIGNED(FLIGHT_PARTITION_SIZE, BIG_SECTOR_SIZE),
              "Invalid place parameter partition. Needs 64KB Alignment");

static_assert(NUM_FLIGHT_PACKETS * sizeof(Packet) < FLIGHT_PARTITION_SIZE);
static_assert(FLIGHT_PARTITION_SIZE % sizeof(Packet) == 0, "Can't write across page boundaries (currently)");

static uint32_t thisBootcount = 0;

struct BootcountPartitionData {
    static constexpr uint8_t MAGIC1[4] = {'A', 'I', 'R', 'B'};
    static constexpr uint8_t MAGIC2[4] = {'R', 'A', 'K', 'E'};
    uint8_t magic1[4];
    uint32_t bootcount;
    uint8_t magic2[4];

    bool IsValid() {
        bool magic1good = memcmp(&magic1[0], &MAGIC1[0], sizeof(MAGIC1)) == 0;
        bool magic2good = memcmp(&magic2[0], &MAGIC2[0], sizeof(MAGIC2)) == 0;
        return magic1good && magic2good;
    }
    void Reset(uint32_t bc) {
        memcpy(&magic1[0], &MAGIC1[0], 4);
        memcpy(&magic2[0], &MAGIC2[0], 4);
        bootcount = bc;
    }
};

int handleBootcount() {
    // read bootcount sectors, calculate bootcount, writeback bootcount
    constexpr off_t a_addr = BOOTCOUNT_PARTITION_OFFSET;
    constexpr off_t b_addr = BOOTCOUNT_PARTITION_OFFSET + SECTOR_SIZE;
    BootcountPartitionData a = {0};
    BootcountPartitionData b = {0};
    int aret = flash_read(flashDevice, a_addr, &a, sizeof(a));
    int bret = flash_read(flashDevice, b_addr, &b, sizeof(b));

    bool agood = (aret == 0) && a.IsValid();
    bool bgood = (bret == 0) && b.IsValid();

    bool writeA = false;
    bool writeB = false;

    uint32_t lastBootcount = 0;

    if (agood) {
        lastBootcount = a.bootcount;
    } else {
        LOG_WRN("Bootcount A partition corrupted or unread, clearing....");
        writeA = true;
    }
    if (bgood) {
        if (b.bootcount > lastBootcount) {
            lastBootcount = b.bootcount;
            writeA = true;
        } else {
            writeB = true;
        }
    } else {
        LOG_WRN("Bootcount B partition corrupted or unread, clearing....");
        writeB = true;
    }

    if (!agood && !bgood) {
        LOG_ERR("No valid bootcount partitions, will initialize to 0");
    }

    if (writeA) {
        int ret = flash_erase(flashDevice, a_addr, SECTOR_SIZE);
        if (ret != 0) {
            LOG_WRN("Failed to erase partition A (%d). Bootcount may be corrupted", ret);
        }
        a.Reset(lastBootcount + 1);
        ret = flash_write(flashDevice, a_addr, &a, sizeof(a));
        if (ret != 0) {
            LOG_ERR("Failed to write partition A  (%d). Bootcount may be corrupted", ret);
        }
    }

    if (writeB) {
        int ret = flash_erase(flashDevice, b_addr, SECTOR_SIZE);
        if (ret != 0) {
            LOG_WRN("Failed to erase partition B (%d). Bootcount may be corrupted", ret);
        }
        b.Reset(lastBootcount + 1);
        ret = flash_write(flashDevice, b_addr, &b, sizeof(b));
        if (ret != 0) {
            LOG_ERR("Failed to write partition B (%d). Bootcount may be corrupted", ret);
        }
    }
    thisBootcount = lastBootcount + 1;
    LOG_INF("Bootcount %d", thisBootcount);
    return 0;
}
extern "C" int storage_init() {
    handleBootcount();
    return 0;
}

namespace NStorage {
uint32_t GetBootcount() { return thisBootcount; }
} // namespace Storage