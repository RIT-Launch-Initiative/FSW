// Launch Includes
#include <launch_core/os/fs.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOGGING_STACK_SIZE 2048
#define MAX_DIR_NAME_LEN   11
#define MAX_FILE_NAME_LEN  3

LOG_MODULE_REGISTER(logging);

static void logging_task(void);
K_THREAD_DEFINE(data_logger, LOGGING_STACK_SIZE, logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

// Message queues
// TODO: Avoid duplicate queues. Fine for now since this isn't too expensive and we have the memory.
// Easier for now since we also need to buffer data before launch
K_MSGQ_DEFINE(ina_logging_msgq, sizeof(power_module_telemetry_t), 50, 4);
K_MSGQ_DEFINE(adc_logging_msgq, sizeof(float), 200, 4);

static void init_logging(l_fs_file_t *p_ina_file, l_fs_file_t *p_adc_file) {
    uint32_t boot_count = l_fs_boot_count_check();

    // Create directory with boot count
    char dir_name[MAX_DIR_NAME_LEN] = "";
    snprintf(dir_name, sizeof(dir_name), "%d", boot_count);
    LOG_INF("Logging files to %s", dir_name);

    // Create filenames
    static char ina_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(ina_file_name, sizeof(ina_file_name), "%s/ina", dir_name);

    static char adc_file_name[MAX_DIR_NAME_LEN + MAX_FILE_NAME_LEN + 1] = "";
    snprintf(adc_file_name, sizeof(adc_file_name), "%s/adc", dir_name);

    // Initialize structs
    const uint32_t ina_samples = 4600;  // 15 samples per second for 5 minutes (rounded to nearest hundred)
    const uint32_t adc_samples = 20000; // 6.6 samples per second for 5 minutes
    static l_fs_file_t ina_file = {
        .fname = ina_file_name,
        .width = sizeof(power_module_telemetry_t),
        .mode = FS_O_RDWR,
        .size = sizeof(power_module_telemetry_t) * ina_samples,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    static l_fs_file_t adc_file = {
        .fname = adc_file_name,
        .width = sizeof(float),
        .mode = FS_O_RDWR,
        .size = sizeof(float) * adc_samples,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };

    LOG_INF("Creating files %s and %s", ina_file_name, adc_file_name);

    // Initialize files
    if (l_fs_init(&ina_file) == 0) {
        p_ina_file = &ina_file;
    }

    if (l_fs_init(&adc_file) == 0) {
        p_adc_file = &adc_file;
    }
}

extern bool logging_enabled;

static void logging_task(void) {
    static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
    static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);

    l_fs_file_t *ina_file = NULL;
    l_fs_file_t *adc_file = NULL;

    power_module_telemetry_t sensor_telemetry;
    float vin_adc_data_v;

    init_logging(&ina_file, &adc_file);

    while (true) {
        if (!logging_enabled) {
            continue;
        }

        if (!k_msgq_get(&ina_logging_msgq, &sensor_telemetry, K_MSEC(10))) {
            LOG_INF("Logged INA219 data");
            gpio_pin_toggle_dt(&led1);
        }

        if (!k_msgq_get(&adc_logging_msgq, &vin_adc_data_v, K_MSEC(3))) {
            LOG_INF("Logged ADC data");
            gpio_pin_toggle_dt(&led3);
        }
    }
}
