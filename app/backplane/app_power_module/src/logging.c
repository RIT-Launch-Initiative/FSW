// Self Include
#include "power_module.h"

// Launch Includes
#include <launch_core/os/fs.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOGGING_STACK_SIZE 4096
#define MAX_DIR_NAME_LEN   10 // 4 number boot count + 5 for "/lfs/" + 1 for end slash
#define MAX_FILE_NAME_LEN  3

#define INA_SAMPLE_COUNT  4600          // 15 samples per second for 5 minutes (rounded to nearest hundred)
#define ADC_SAMPLE_COUNT  16000         // 50 samples per second for 5 minutes (rounded to nearest thousand)
#define GNSS_SAMPLE_COUNT (10 * 60 / 2) // 1 samples every 2 seconds for 10 minutes
LOG_MODULE_REGISTER(logging);

static void logging_task(void);
K_THREAD_DEFINE(data_logger, LOGGING_STACK_SIZE, logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

// Message queues
// TODO: Avoid duplicate queues. Fine for now since this isn't too expensive and we have the memory.
// Easier for now since we also need to buffer data before launch
K_MSGQ_DEFINE(ina_logging_msgq, sizeof(timed_power_module_telemetry_t), 50, 4);
K_MSGQ_DEFINE(adc_logging_msgq, sizeof(timed_adc_telemetry_t), 200, 4);
K_MSGQ_DEFINE(gnss_logging_msgq, sizeof(l_gnss_time_sync_t), 200, 4);

#ifdef CONFIG_SEND_LAST_LOG
#include <launch_core/net/tftp.h>
static void send_last_log(const uint32_t boot_count_to_get) {
    LOG_INF("Attempting to send last logs over TFTP");
    // Open /lfs/current_boot_count-1 directory
    char dir_name[MAX_DIR_NAME_LEN] = "";
    snprintf(dir_name, sizeof(dir_name), "/lfs/%d", boot_count_to_get);

    // Setup output file names
    char ina_output_file_name[MAX_FILE_NAME_LEN + 4] = "";
    snprintf(ina_output_file_name, sizeof(ina_output_file_name), "ina_%d", boot_count_to_get);

    char adc_output_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(adc_output_file_name, sizeof(adc_output_file_name), "adc_%d", boot_count_to_get);
    LOG_INF("Writing last logs to %s and %s", ina_output_file_name, adc_output_file_name);

    // Read INA into buffer
    char ina_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(ina_file_name, sizeof(ina_file_name), "%s/ina", dir_name);
    l_fs_file_t ina_file = {
        .fname = ina_file_name,
        .width = sizeof(timed_power_module_telemetry_t),
        .mode = SLOG_ONCE,
        .size = sizeof(timed_power_module_telemetry_t) * INA_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    char adc_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(adc_file_name, sizeof(adc_file_name), "%s/adc", dir_name);
    l_fs_file_t adc_file = {
        .fname = adc_file_name,
        .width = sizeof(timed_adc_telemetry_t),
        .mode = SLOG_ONCE,
        .size = sizeof(timed_adc_telemetry_t) * ADC_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    static char gnss_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(gnss_file_name, sizeof(gnss_file_name), "%s/gns", dir_name);
    static l_fs_file_t gnss_file = {
        .fname = gnss_file_name,
        .width = sizeof(l_gnss_time_sync_t),
        .mode = SLOG_ONCE,
        .size = sizeof(l_gnss_time_sync_t) * GNSS_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    timed_power_module_telemetry_t ina_data[INA_SAMPLE_COUNT] = {0};
    size_t ina_log_file_size = l_fs_file_size(&ina_file);
    LOG_INF("Previous INA219 Log File Size: %d", ina_log_file_size);
    if ((l_fs_init(&ina_file) == 0) && fs_read(&ina_file.file, &ina_data, ina_log_file_size)) {
        l_tftp_init_and_put(L_DEFAULT_SERVER_IP, ina_output_file_name, (uint8_t *) &ina_data, ina_log_file_size);
        l_fs_close(&ina_file);
    } else {
        LOG_ERR("Failed to read INA data from file.");
    }

    timed_adc_telemetry_t adc_data[ADC_SAMPLE_COUNT] = {0};
    size_t adc_log_file_size = l_fs_file_size(&adc_file);
    LOG_INF("Previous ADC Log File Size: %d", adc_log_file_size);
    if ((l_fs_init(&adc_file) == 0) && fs_read(&adc_file.file, &adc_data, adc_log_file_size)) {
        l_tftp_init_and_put(L_DEFAULT_SERVER_IP, adc_output_file_name, (uint8_t *) &adc_data, adc_log_file_size);
        l_fs_close(&adc_file);
    } else {
        LOG_ERR("Failed to read ADC data from file.");
    }

    timed_sensor_module_ten_hz_telemetry_t gnss_data[TEN_HZ_SAMPLE_COUNT] = {0};
    size_t gnss_log_file_size = l_fs_file_size(&gnss_file);
    LOG_INF("Previous gnss Log File Size: %d", gnss_log_file_size);
    if ((l_fs_init(&gnss_file) == 0) && fs_read(&gnss_file.file, &gnss_data, gnss_log_file_size)) {
        l_tftp_init_and_put(L_DEFAULT_SERVER_IP, gnss_file_name, (uint8_t *) &gnss_data, gnss_log_file_size);
        l_fs_close(&gnss_file);
    } else {
        LOG_ERR("Failed to read gnss data from file.");
    }
}
#endif

static void init_logging(l_fs_file_t **p_ina_file, l_fs_file_t **p_adc_file, l_fs_file_t **p_gnss_file) {
    uint32_t boot_count = l_fs_boot_count_check();
#ifdef CONFIG_SEND_LAST_LOG
    send_last_log(boot_count);
#endif
    // Create directory with boot count
    char dir_name[MAX_DIR_NAME_LEN] = "";
    snprintf(dir_name, sizeof(dir_name), "/lfs/%d", boot_count);
    LOG_INF("Logging files to %s", dir_name);
    fs_mkdir(dir_name);

    // Create filenames
    static char ina_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(ina_file_name, sizeof(ina_file_name), "%s/ina", dir_name);

    static char adc_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(adc_file_name, sizeof(adc_file_name), "%s/adc", dir_name);

    static char gnss_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(gnss_file_name, sizeof(gnss_file_name), "%s/gns", dir_name);

    // Initialize structs
    static l_fs_file_t ina_file = {
        .fname = ina_file_name,
        .width = sizeof(timed_power_module_telemetry_t),
        .mode = SLOG_ONCE,
        .size = sizeof(timed_power_module_telemetry_t) * INA_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    static l_fs_file_t adc_file = {
        .fname = adc_file_name,
        .width = sizeof(timed_adc_telemetry_t),
        .mode = SLOG_ONCE,
        .size = sizeof(timed_adc_telemetry_t) * ADC_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    static l_fs_file_t gnss_file = {
        .fname = gnss_file_name,
        .width = sizeof(l_gnss_time_sync_t),
        .mode = SLOG_ONCE,
        .size = sizeof(l_gnss_time_sync_t) * GNSS_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    LOG_INF("Creating files %s, %s, and %s", ina_file_name, adc_file_name, gnss_file_name);

    // Initialize files
    if (l_fs_init(&ina_file) == 0) {
        LOG_INF("Successfully created file for storing INA219 data.");
        *p_ina_file = &ina_file;
    }

    if (l_fs_init(&adc_file) == 0) {
        LOG_INF("Successfully created file for storing ADC data.");
        *p_adc_file = &adc_file;
    }
    if (l_fs_init(&gnss_file) == 0) {
        LOG_INF("Successfully created file for storing GNSS data.");
        *p_adc_file = &gnss_file;
    }
}

extern bool logging_enabled;

static void logging_task(void) {
    static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
    static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);

    l_fs_file_t *ina_file = NULL;
    l_fs_file_t *adc_file = NULL;
    l_fs_file_t *gnss_file = NULL;

    timed_power_module_telemetry_t sensor_telemetry;
    timed_adc_telemetry_t vin_adc_data_v;
    l_gnss_time_sync_t gnss_data;

    init_logging(&ina_file, &adc_file, &gnss_file);

    bool ina_out_of_space = false;
    bool adc_out_of_space = false;
    bool gnss_out_of_space = false;

    while (true) {
        if (!logging_enabled) {
            continue;
        }

        if (!k_msgq_get(&ina_logging_msgq, &sensor_telemetry, K_MSEC(10)) && (ina_file != NULL) &&
            (!ina_out_of_space)) {
            LOG_INF("Logged INA219 data");
            gpio_pin_toggle_dt(&led1);

            int32_t err_flag = 0;
            l_fs_write(ina_file, (const uint8_t *) &sensor_telemetry, &err_flag);
            ina_out_of_space = err_flag == -ENOSPC;
            LOG_INF("%d", err_flag);
        }

        if (!k_msgq_get(&adc_logging_msgq, &vin_adc_data_v, K_MSEC(3)) && (adc_file != NULL) && (!adc_out_of_space)) {
            LOG_INF("Logged ADC data");
            gpio_pin_toggle_dt(&led3);

            int32_t err_flag = 0;
            l_fs_write(adc_file, (const uint8_t *) &vin_adc_data_v, &err_flag);
            adc_out_of_space = err_flag == -ENOSPC;
            LOG_INF("%d", err_flag);
        }
        if (!k_msgq_get(&gnss_logging_msgq, &gnss_data, K_MSEC(3)) && (gnss_file != NULL) && (!gnss_out_of_space)) {
            LOG_INF("Logged GNSS data");
            gpio_pin_toggle_dt(&led3);

            int32_t err_flag = 0;
            l_fs_write(gnss_file, (const uint8_t *) &gnss_data, &err_flag);
            gnss_out_of_space = err_flag == -ENOSPC;
            LOG_INF("%d", err_flag);
        }

        if (ina_out_of_space && adc_out_of_space && gnss_out_of_space) {
            LOG_ERR("Out of space on both INA219, ADC and GNSS files. Stopping logging.");
            logging_enabled = false;
            l_fs_close(ina_file);
            l_fs_close(adc_file);
            l_fs_close(gnss_file);
            return;
        }
    }
}
