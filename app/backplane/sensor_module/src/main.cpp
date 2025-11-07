/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "c_sensor_module.h"

#include <f_core/os/n_rtos.h>
#include <zephyr/init.h>
#include <zephyr/shell/shell.h>

struct boot_count_info {
    char magic[4];
    uint32_t bootcount;
    char magic2[4];
};

const struct device* flash_dev = DEVICE_DT_GET(DT_CHOSEN(storage));
static constexpr struct boot_count_info init_bootcount_info = {{'d', 'e', 'a', 'n'}, 0, {'d', 'e', 'a', 'n'}};

int load_bootcount_info(size_t offset, struct boot_count_info* bootcount_info) {
    int ret = flash_read(flash_dev, offset, bootcount_info, sizeof(struct boot_count_info));
    if (ret != 0) {
        printk("Failed to read bootcount: %d\n", ret);
        return ret;
    }

    bool match = true;
    for (size_t i = 0; i < sizeof(init_bootcount_info.magic); i++) {
        if (init_bootcount_info.magic[i] != bootcount_info->magic[i] ||
            init_bootcount_info.magic2[i] != bootcount_info->magic2[i]) {
            match = false;
        }
    }
    if (!match) {
        printk("Failed to load correct magic bytes for bootcount\n");
        printk("%02x %02x %02x %02x %u %02x %02x %02x %02x\n", bootcount_info->magic[0], bootcount_info->magic[1],
               bootcount_info->magic[2], bootcount_info->magic[3], bootcount_info->bootcount, bootcount_info->magic2[0], bootcount_info->magic2[1],
               bootcount_info->magic2[2], bootcount_info->magic2[3]);
        return -1;
    }
    return 0;
}
// PRECONDITION, load_bootcount_info has been called
int increment_and_save_bootcount(struct boot_count_info* bootcount_info) {
    bootcount_info->bootcount++;
    // A
    {
        int ret = flash_erase(flash_dev, 0, 4096);

        if (ret != 0) {
            printk("FAILED TO ERASE BOOTCOUNT PAGE. ERRORS INBOUND\n");
            return ret;
        }

        ret = flash_write(flash_dev, 0, bootcount_info, sizeof(struct boot_count_info));

        if (ret != 0) {
            printk("FAILED TO WRITE BOOTCOUNT PAGE. ERRORS INBOUND\n");
            return ret;
        }
    }
    // B
    {
        int ret = flash_erase(flash_dev, 4096, 4096);

        if (ret != 0) {
            printk("FAILED TO ERASE BOOTCOUNT PAGE. ERRORS INBOUND\n");
            return ret;
        }

        ret = flash_write(flash_dev, 4096, bootcount_info, sizeof(struct boot_count_info));

        if (ret != 0) {
            printk("FAILED TO WRITE BOOTCOUNT PAGE. ERRORS INBOUND\n");
            return ret;
        }
    }
    return 0;
}

static struct boot_count_info bc_a = {};
static struct boot_count_info bc_b = {};
static struct boot_count_info real_bootcount_info = {};
size_t sensor_mod_bootcount = 0;

int bootcount_before_anything() {
    int a_ret = load_bootcount_info(0, &bc_a);
    int b_ret = load_bootcount_info(4096, &bc_b);
    bool a_ok = a_ret == 0;
    bool b_ok = b_ret == 0;
    if (a_ok && b_ok) {
        if (bc_a.bootcount > bc_b.bootcount) {
            printk("BC B Ok\n");
            real_bootcount_info = bc_a;
        } else {
            printk("BC B Ok\n");
            real_bootcount_info = bc_b;
        }
    } else if (a_ok) {
        printk("BC A Ok\n");
        real_bootcount_info = bc_a;
    } else if (b_ok) {
        printk("BC B Ok\n");
        real_bootcount_info = bc_b;
    } else {
        printk("All bootcounts corrupt. Using default\n");
        real_bootcount_info = init_bootcount_info;
    }

    printk("Boot: %u\n", real_bootcount_info.bootcount);
    sensor_mod_bootcount = real_bootcount_info.bootcount;
    return increment_and_save_bootcount(&real_bootcount_info);
}
// if you lower this too much it blows up
// its dependent on other post kernel stuff
#define BOOTCOUNT_PRIO 90

SYS_INIT(bootcount_before_anything, POST_KERNEL, BOOTCOUNT_PRIO);

static CSensorModule sensorModule{};
int main() {

    sensorModule.AddTenantsToTasks();
    sensorModule.AddTasksToRtos();
    sensorModule.SetupCallbacks();

    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(900));
    NRtos::StopRtos();
    sensorModule.Cleanup();
#endif

    return 0;
}

int cmd_doboost(const struct shell* shell, size_t argc, char** argv) {
    sensorModule.Controller().SubmitEvent(Sources::LowGImu, Events::Boost);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(control_subcmds, SHELL_CMD(boost, NULL, "Send LSM Boost", cmd_doboost),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(control, &control_subcmds, "Control Commands", NULL);
