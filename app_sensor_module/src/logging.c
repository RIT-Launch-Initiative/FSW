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

#define HUN_HZ_SAMPLE_COUNT 30000 // 100 samples per second for 5 minutes (rounded to nearest hundred)
#define TEN_HZ_SAMPLE_COUNT 300   // 1 sample per second for 5 minutes

LOG_MODULE_REGISTER(logging);

static void logging_task(void);
K_THREAD_DEFINE(data_logger, LOGGING_STACK_SIZE, logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

// Message queues
K_MSGQ_DEFINE(hun_hz_logging_msgq, sizeof(timed_sensor_module_hundred_hz_telemetry_t), 50, 4);
K_MSGQ_DEFINE(ten_hz_logging_msgq, sizeof(timed_sensor_module_ten_hz_telemetry_t), 200, 4);

#ifdef CONFIG_DEBUG
#include <launch_core/net/tftp.h>

static void send_last_log(const uint32_t boot_count_to_get) {
    LOG_INF("Attempting to send last logs over TFTP");
    // Open /lfs/current_boot_count-1 directory
    char dir_name[MAX_DIR_NAME_LEN] = "";
    snprintf(dir_name, sizeof(dir_name), "/lfs/%d", boot_count_to_get);

    // Setup output file names
    char hun_hz_output_file_name[MAX_FILE_NAME_LEN + 4] = "";
    snprintf(hun_hz_output_file_name, sizeof(hun_hz_output_file_name), "hun_%d", boot_count_to_get);

    char ten_hz_output_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(ten_hz_output_file_name, sizeof(ten_hz_output_file_name), "ten_%d", boot_count_to_get);
    LOG_INF("Writing last logs to %s and %s", hun_hz_output_file_name, ten_hz_output_file_name);

    // Read INA into buffer
    char hun_hz_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(hun_hz_file_name, sizeof(hun_hz_file_name), "%s/hun", dir_name);
    l_fs_file_t hun_hz_file = {
        .fname = hun_hz_file_name,
        .width = sizeof(timed_sensor_module_hundred_hz_telemetry_t),
        .mode = SLOG_ONCE,
        .size = sizeof(timed_sensor_module_hundred_hz_telemetry_t) * HUN_HZ_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    char ten_hz_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(ten_hz_file_name, sizeof(ten_hz_file_name), "%s/ten", dir_name);
    l_fs_file_t ten_hz_file = {
        .fname = ten_hz_file_name,
        .width = sizeof(timed_sensor_module_ten_hz_telemetry_t),
        .mode = SLOG_ONCE,
        .size = sizeof(timed_sensor_module_ten_hz_telemetry_t) * TEN_HZ_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    timed_sensor_module_hundred_hz_telemetry_t hun_hz_data[HUN_HZ_SAMPLE_COUNT] = {0};
    size_t hun_hz_log_file_size = l_fs_file_size(&hun_hz_file);
    LOG_INF("Previous 100Hz Log File Size: %d", hun_hz_log_file_size);
    if ((l_fs_init(&hun_hz_file) == 0) && fs_read(&hun_hz_file.file, &hun_hz_data, hun_hz_log_file_size)) {
        l_tftp_init_and_put(L_DEFAULT_SERVER_IP, hun_hz_output_file_name, (uint8_t*) &hun_hz_data,
                            hun_hz_log_file_size);
        l_fs_close(&hun_hz_file);
    } else {
        LOG_ERR("Failed to read INA data from file.");
    }

    timed_sensor_module_ten_hz_telemetry_t ten_hz_data[TEN_HZ_SAMPLE_COUNT] = {0};
    size_t ten_hz_log_file_size = l_fs_file_size(&ten_hz_file);
    LOG_INF("Previous 10Hz Log File Size: %d", ten_hz_log_file_size);
    if ((l_fs_init(&ten_hz_file) == 0) && fs_read(&ten_hz_file.file, &ten_hz_data, ten_hz_log_file_size)) {
        l_tftp_init_and_put(L_DEFAULT_SERVER_IP, ten_hz_output_file_name, (uint8_t*) &ten_hz_data,
                            ten_hz_log_file_size);
        l_fs_close(&ten_hz_file);
    } else {
        LOG_ERR("Failed to read 10Hz data from file.");
    }
}
#endif

static void init_logging(l_fs_file_t** p_hun_hz_file, l_fs_file_t** p_ten_hz_file) {
    uint32_t boot_count = l_fs_boot_count_check();
#ifdef CONFIG_DEBUG
    send_last_log(boot_count);
#endif
    // Create directory with boot count
    char dir_name[MAX_DIR_NAME_LEN] = "";
    snprintf(dir_name, sizeof(dir_name), "/lfs/%d", boot_count);
    LOG_INF("Logging files to %s", dir_name);
    fs_mkdir(dir_name);

    // Create filenames
    static char hun_hz_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(hun_hz_file_name, sizeof(hun_hz_file_name), "%s/hun", dir_name);

    static char ten_hz_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(ten_hz_file_name, sizeof(ten_hz_file_name), "%s/ten", dir_name);

    // Initialize structs
    static l_fs_file_t hun_hz_file = {
        .fname = hun_hz_file_name,
        .width = sizeof(timed_sensor_module_hundred_hz_telemetry_t),
        .mode = SLOG_ONCE,
        .size = sizeof(timed_sensor_module_hundred_hz_telemetry_t) * HUN_HZ_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    static l_fs_file_t ten_hz_file = {
        .fname = ten_hz_file_name,
        .width = sizeof(timed_sensor_module_ten_hz_telemetry_t),
        .mode = SLOG_ONCE,
        .size = sizeof(timed_sensor_module_ten_hz_telemetry_t) * TEN_HZ_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    LOG_INF("Creating files %s and %s", hun_hz_file_name, ten_hz_file_name);

    // Initialize files
    if (l_fs_init(&hun_hz_file) == 0) {
        LOG_INF("Successfully created file for storing 100Hz data.");
        *p_hun_hz_file = &hun_hz_file;
    }

    if (l_fs_init(&ten_hz_file) == 0) {
        LOG_INF("Successfully created file for storing 10Hz data.");
        *p_ten_hz_file = &ten_hz_file;
    }
}

extern bool logging_enabled;

static void logging_task(void) {
    l_fs_file_t* hun_hz_file = NULL;
    l_fs_file_t* ten_hz_file = NULL;

    timed_sensor_module_hundred_hz_telemetry_t hun_hz_telem;
    timed_sensor_module_ten_hz_telemetry_t ten_hz_telem;

    init_logging(&hun_hz_file, &ten_hz_file);

    bool hun_hz_out_of_space = false;
    bool ten_hz_out_of_space = false;

    while (true) {
        if (!logging_enabled) {
            continue;
        }

        if (!k_msgq_get(&hun_hz_logging_msgq, &hun_hz_telem, K_MSEC(10)) && (hun_hz_file != NULL) &&
            (!hun_hz_out_of_space)) {
            LOG_INF("Logged 100Hz data");

            int32_t err_flag = 0;
            l_fs_write(hun_hz_file, (const uint8_t*) &hun_hz_telem, &err_flag);
            hun_hz_out_of_space = err_flag == -ENOSPC;
            LOG_INF("%d", err_flag);
        }

        if (!k_msgq_get(&ten_hz_logging_msgq, &ten_hz_telem, K_MSEC(3)) && (ten_hz_file != NULL) &&
            (!ten_hz_out_of_space)) {
            LOG_INF("Logged 10Hz data");

            int32_t err_flag = 0;
            l_fs_write(ten_hz_file, (const uint8_t*) &ten_hz_telem, &err_flag);
            ten_hz_out_of_space = err_flag == -ENOSPC;
            LOG_INF("%d", err_flag);
        }

        if (hun_hz_out_of_space && ten_hz_out_of_space) {
            LOG_ERR("Out of space on both 100Hz and 10Hz files. Stopping logging.");
            logging_enabled = false;
            l_fs_close(hun_hz_file);
            l_fs_close(ten_hz_file);
            return;
        }
    }
}
