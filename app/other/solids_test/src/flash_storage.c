#include "flash_storage.h"

#include "adc_reading.h"
#include "config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdalign.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(flash_storage, LOG_LEVEL_INF);

#define SPI_FLASH_ADDR       0x00000000
#define FLASH_METADATA_ADDR  SPI_FLASH_ADDR
#define PAGE_SIZE            256
#define SAMPLE_PER_PAGE      (PAGE_SIZE / sizeof(struct adc_sample))
#define SPI_FLASH_BLOCK_SIZE (64 * 2048) // 128KB
#define FLASH_METADATA_SIZE  (4 * 1024)  // 4KB
#define SPI_FLASH_START_ADDR (FLASH_METADATA_ADDR + FLASH_METADATA_SIZE)

enum storage_event { BEGIN_STORAGE, END_STORAGE };

K_MSGQ_DEFINE(storage_control_queue, sizeof(enum storage_event), 5, 1);

K_MSGQ_DEFINE(adc_data_queue, sizeof(struct adc_sample), 1000, alignof(struct adc_sample));

static const struct device *flash_dev = DEVICE_DT_GET(DT_ALIAS(storage));
static uint32_t current_test_number = 0;
static off_t current_write_addr = 0;

static void flash_storage_thread_entry(void);

K_THREAD_DEFINE(storage_thread, 2048, flash_storage_thread_entry, NULL, NULL, NULL, STORAGE_THREAD_PRIORITY, 0, 1000);

// Check if flash block is all 0xFF
static bool flash_block_is_empty(off_t addr) {
    uint8_t buf[16];
    if (flash_read(flash_dev, addr, buf, sizeof(buf)) < 0) {
        LOG_ERR("Flash read failed");
        return false;
    }

    for (int i = 0; i < sizeof(buf); i++) {
        if (buf[i] != 0xFF) {
            return false;
        }
    }
    return true;
}

// Get and update current test number
static void load_metadata() {
    uint8_t buf[4];
    if (flash_read(flash_dev, FLASH_METADATA_ADDR, buf, sizeof(buf)) < 0) {
        LOG_ERR("Failed to read metadata");
        current_test_number = 0;
        return;
    }

    // If all are erased (0xFF), assume no tests yet
    if (buf[0] == 0xFF && buf[1] == 0xFF && buf[2] == 0xFF && buf[3] == 0xFF) {
        current_test_number = 0;
    } else {
        memcpy(&current_test_number, buf, sizeof(current_test_number));
    }

    LOG_INF("Loaded test number %d", current_test_number);
}

static void save_metadata() {
    int ret;
    ret = flash_erase(flash_dev, FLASH_METADATA_ADDR, FLASH_METADATA_SIZE);

    if (ret < 0) {
        LOG_ERR("flash_erase(metadata) failed: %d", ret);
        return;
    }

    ret = flash_write(flash_dev, FLASH_METADATA_ADDR, &current_test_number, sizeof(current_test_number));

    if (ret < 0) {
        LOG_ERR("flash_write(metadata) failed: %d", ret);
    } else {
        LOG_INF("Saved current_test_number = %u", current_test_number);
    }
}

static off_t get_test_block_addr(uint32_t test_index) {
    return (off_t) (SPI_FLASH_START_ADDR + (test_index * SPI_FLASH_BLOCK_SIZE));
}

static void flash_storage_thread_entry() {
    enum storage_event event;

    if (!device_is_ready(flash_dev)) {
        LOG_ERR("Flash device not ready");
        return;
    }

    // Get current test number
    load_metadata();

    while (1) {
        // Wait for start command
        k_msgq_get(&storage_control_queue, &event, K_FOREVER);
        if (event != BEGIN_STORAGE) continue;

        if (current_test_number >= MAX_TESTS) {
            LOG_ERR("Maximum number of test reached");
            continue;
        }

        off_t test_block_addr = get_test_block_addr(current_test_number);
        LOG_INF("Starting test %u at addr 0x%08lx", current_test_number, (long) test_block_addr);

        // Erase block
        int ret = flash_erase(flash_dev, test_block_addr, SPI_FLASH_BLOCK_SIZE);
        if (ret < 0) {
            LOG_ERR("flash_erase(test block) failed: %d", ret);
            continue;
        }

        current_write_addr = test_block_addr;

        static struct adc_sample page[SAMPLE_PER_PAGE] = {0};
        size_t i = 0;
        size_t pages = 0;
        while (1) {
            if (k_msgq_get(&storage_control_queue, &event, K_NO_WAIT) == 0 && event == END_STORAGE) {
                LOG_INF("Test %d complete", current_test_number);
                current_test_number++;
                save_metadata();
                break;
            }
            struct adc_sample *target = &page[i % SAMPLE_PER_PAGE];
            if (k_msgq_get(&adc_data_queue, target, K_MSEC(50)) == 0) {
                i++;
            }
            if (i % SAMPLE_PER_PAGE == 0 && i != 0) {
                int ret = flash_write(flash_dev, current_write_addr, page, PAGE_SIZE);
                if (ret < 0) {
                    LOG_ERR("Flash write failed (%d)", ret);
                } else {
                    current_write_addr += PAGE_SIZE;
                    pages++;
                }
            }
        }
        size_t j = i % SAMPLE_PER_PAGE;
        while (j < SAMPLE_PER_PAGE) {
            page[j] = (struct adc_sample) {0xFFFFFFFF, 0xFFFFFFFF};
            j++;
        }
        ret = flash_write(flash_dev, current_write_addr, page, PAGE_SIZE);
        if (ret < 0) {
            LOG_ERR("Final page flash write failed (%d)", ret);
        } else {
            current_write_addr += PAGE_SIZE;
            pages++;
        }

        LOG_INF("Saved %u packets to %u pages", i, pages);
    }
}

int start_flash_storage() {
    enum storage_event event = BEGIN_STORAGE;
    return k_msgq_put(&storage_control_queue, &event, K_FOREVER);
}

void stop_flash_storage() {
    enum storage_event event = END_STORAGE;
    k_msgq_put(&storage_control_queue, &event, K_FOREVER);
}

int flash_erase_all();

int flash_dump_one(const struct shell *shell, uint32_t test_index) {
    struct adc_sample sample;

    if (!device_is_ready(flash_dev)) {
        shell_error(shell, "Flash not ready");
        return -1;
    }

    off_t block_addr = get_test_block_addr(test_index);
    if (flash_block_is_empty(block_addr)) {
        shell_print(shell, "Flash block empty");
        return 0;
    }

    shell_print(shell, "Dumping Test %d:", test_index);

    for (int i = 0; i < (SPI_FLASH_BLOCK_SIZE / sizeof(sample)); i++) {
        if (flash_read(flash_dev, block_addr, &sample, sizeof(sample)) < 0) {
            shell_print(shell, "Flash read failed");
            break;
        }
        if (sample.value == 0xFFFFFFFF && sample.timestamp == 0xFFFFFFFF) {
            shell_print(shell, "Flash block unwritten. Read %d packets", i);
            break;
        }
        shell_print(shell, "%u,%u", sample.timestamp, sample.value);
        block_addr += sizeof(sample);
    }

    return 0;
}

int flash_dump_all(const struct shell *shell) {
    if (!device_is_ready(flash_dev)) {
        shell_error(shell, "Flash not ready");
        return -1;
    }

    for (uint32_t test_index = 0; test_index < MAX_TESTS; test_index++) {
        flash_dump_one(shell, test_index);
    }

    return 0;
}

int flash_erase_all(const struct shell *shell) {
    for (uint32_t i = 0; i < MAX_TESTS; i++) {
        off_t curr_add = get_test_block_addr(i);
        int ret = flash_erase(flash_dev, curr_add, SPI_FLASH_BLOCK_SIZE);
        if (ret < 0) {
            shell_print(shell, "flash_erase failed: %d", ret);
            continue;
        } else {
            shell_error(shell, "Flash block %d erased", i);
        }
    }

    current_test_number = 0;
    save_metadata();

    return 0;
}
