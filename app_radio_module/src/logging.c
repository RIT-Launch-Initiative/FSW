#ifndef CONFIG_RADIO_MODULE_RECEIVER
// Launch Includes
#include <launch_core/os/fs.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOGGING_STACK_SIZE 4096
#define MAX_DIR_NAME_LEN   10 // 4 number boot count + 5 for "/lfs/" + 1 for end slash
#define MAX_FILE_NAME_LEN  4

#define GNSS_SAMPLE_COUNT 300 // 1 samples every 2 seconds for 10 minutes

LOG_MODULE_REGISTER(logging);

static void logging_task(void);
K_THREAD_DEFINE(data_logger, LOGGING_STACK_SIZE, logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

// Message queues
K_MSGQ_DEFINE(gnss_logging_msgq, sizeof(l_gnss_data_t), 10, 4);

#ifdef CONFIG_DEBUG
#include <launch_core/net/tftp.h>

static void send_last_log(const uint32_t boot_count_to_get) {
    LOG_INF("Attempting to send last logs over TFTP");
    // Open /lfs/current_boot_count-1 directory
    char dir_name[MAX_DIR_NAME_LEN] = "";
    snprintf(dir_name, sizeof(dir_name), "/lfs/%d", boot_count_to_get);

    // Setup output file names
    char gnss_output_file_name[MAX_FILE_NAME_LEN + 4] = "";
    snprintf(gnss_output_file_name, sizeof(gnss_output_file_name), "gnss_%d", boot_count_to_get);

    // Read GNSS into buffer
    char gnss_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(gnss_file_name, sizeof(gnss_file_name), "%s/gnss", dir_name);
    l_fs_file_t gnss_file = {
        .fname = gnss_file_name,
        .width = sizeof(l_gnss_data_t),
        .mode = SLOG_ONCE,
        .size = sizeof(l_gnss_data_t) * GNSS_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    l_gnss_data_t gnss_data[GNSS_SAMPLE_COUNT] = {0};
    size_t gnss_log_file_size = l_fs_file_size(&gnss_file);
    LOG_INF("Previous GNSS Log File Size: %d", gnss_log_file_size);
    if ((l_fs_init(&gnss_file) == 0) && fs_read(&gnss_file.file, &gnss_data, gnss_log_file_size)) {
        l_tftp_init_and_put(L_DEFAULT_SERVER_IP, gnss_output_file_name, (uint8_t*) &gnss_data, gnss_log_file_size);
        l_fs_close(&gnss_file);
    } else {
        LOG_ERR("Failed to read GNSS data from file.");
    }
}
#endif

static void init_logging(l_fs_file_t** p_gnss_file) {
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
    static char gnss_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(gnss_file_name, sizeof(gnss_file_name), "%s/gnss", dir_name);

    // Initialize structs
    static l_fs_file_t gnss_file = {
        .fname = gnss_file_name,
        .width = sizeof(l_gnss_data_t),
        .mode = SLOG_ONCE,
        .size = sizeof(l_gnss_data_t) * GNSS_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    LOG_INF("Creating file %s", gnss_file_name);

    // Initialize files
    if (l_fs_init(&gnss_file) == 0) {
        LOG_INF("Successfully created file for storing GNSS data.");
        *p_gnss_file = &gnss_file;
    }
}

extern bool logging_enabled;

static void logging_task(void) {
    l_fs_file_t* gnss_file = NULL;

    l_gnss_data_t gnss_data;

    init_logging(&gnss_file);

    bool gnss_out_of_space = false;

    while (true) {
        if (!logging_enabled) {
            continue;
        }

        if (!k_msgq_get(&gnss_logging_msgq, &gnss_data, K_MSEC(10)) && (gnss_file != NULL) &&
            (!gnss_out_of_space)) {
            LOG_INF("Logged GNSS data");

            int32_t err_flag = 0;
            l_fs_write(gnss_file, (const uint8_t*) &gnss_data, &err_flag);
            gnss_out_of_space = err_flag == -ENOSPC;
            LOG_INF("%d", err_flag);
        }

        if (gnss_out_of_space) {
            LOG_ERR("Out of space logging GNSS. Stopping logging.");
            logging_enabled = false;
            l_fs_close(gnss_file);
            return;
        }
    }
}
#endif
