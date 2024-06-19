// Self Include
#include "sensor_module.h"

// Launch Includes
#include <launch_core/os/fs.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOGGING_STACK_SIZE 4096
#define MAX_DIR_NAME_LEN   10 // 4 number boot count + 5 for "/lfs/" + 1 for end slash
#define MAX_FILE_NAME_LEN  3

#define HUN_HZ_SAMPLE_COUNT 30000         // 100 samples per second for 5 minutes (rounded to nearest hundred)

LOG_MODULE_REGISTER(logging);

static void logging_task(void);
K_THREAD_DEFINE(data_logger, LOGGING_STACK_SIZE, logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(25), 0, 1000);

// Message queues
K_MSGQ_DEFINE(telem_logging_msgq, sizeof(sensor_module_telemetry_t), 1000, 4);

static void init_logging(l_fs_file_t** p_telem_file) {
    uint32_t boot_count = l_fs_boot_count_check();

    // Create directory with boot count
    char dir_name[MAX_DIR_NAME_LEN] = "";
    snprintf(dir_name, sizeof(dir_name), "/lfs/%d", boot_count);
    LOG_INF("Logging files to %s", dir_name);
    fs_mkdir(dir_name);

    // Create filenames
    static char telem_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(telem_file_name, sizeof(telem_file_name), "%s/hun", dir_name);

    // Initialize structs
    static l_fs_file_t telem_file = {
        .fname = telem_file_name,
        .width = sizeof(sensor_module_telemetry_t),
        .mode = SLOG_ONCE,
        .size = sizeof(sensor_module_telemetry_t) * HUN_HZ_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    // Initialize files
    if (l_fs_init(&telem_file) == 0) {
        LOG_INF("Successfully created file for storing 100Hz data.");
        *p_telem_file = &telem_file;
    }
}

static void logging_task(void) {
    l_fs_file_t* telem_file = NULL;

    sensor_module_telemetry_t telem;

    while (telem_file == NULL) {
        init_logging(&telem_file);
        k_msleep(1000);
    }

    bool out_of_space = false;

    while (true) {
        if (!k_msgq_get(&telem_logging_msgq, &telem, K_MSEC(10)) && (telem_file != NULL)) {
            LOG_INF("Logged data");

            int32_t err_flag = 0;
            l_fs_write(telem_file, (const uint8_t*) &telem, &err_flag);
            out_of_space = err_flag == -ENOSPC;
        }

        if (out_of_space) {
            l_fs_close(telem_file);
            return;
        }
    }
}
