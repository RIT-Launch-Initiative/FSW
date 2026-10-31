#include <cerrno>
#include <cstdint>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(main);

#define DEFAULT_VAL 0

static uint8_t storedVal = DEFAULT_VAL;
static uint8_t storedVal2 = DEFAULT_VAL;

static int test_settings_get(const char *name, char *val_buffer, int val_len_max) {
    const char *next;

    if (settings_name_steq(name, "test_child", &next) && !next) {
        if (val_len_max != sizeof(storedVal)) {
            return -EINVAL;
        }

        memcpy(val_buffer, &storedVal, sizeof(storedVal));
        return sizeof(storedVal);
    }

    if (settings_name_steq(name, "test_child2", &next) && !next) {
        if (val_len_max != sizeof(storedVal2)) {
            return -EINVAL;
        }

        memcpy(val_buffer, &storedVal2, sizeof(storedVal2));
        return sizeof(storedVal2);
    }

    return -ENOENT;
}

// read test setting from flash
static int test_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    int rc;

    if (settings_name_steq(name, "test_child", &next) && !next) {
        // if the read storage doesn't match, be sad
        if (len != sizeof(storedVal)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &storedVal, sizeof(storedVal));

        if (rc >= 0) {
            // rc contains value length, so it'll
            // be 0 if key-value is properly read
            return 0;
        }

        return rc;
    }

    if (settings_name_steq(name, "test_child2", &next) && !next) {
        // if the read storage doesn't match, be sad
        if (len != sizeof(storedVal2)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &storedVal2, sizeof(storedVal2));

        if (rc >= 0) {
            // rc contains value length, so it'll
            // be 0 if key-value is properly read
            return 0;
        }

        return rc;
    }

    return -ENOENT;
}

static int test_settings_commit() {
    LOG_INF("All settings have been loaded!");
    return 0;
}

static int test_settings_export(int (*storage_func)(const char *name, const void *value, size_t val_len)) {
    int res1 = storage_func("test/test_child", &storedVal, sizeof(storedVal));
    int res2 = storage_func("test/test_child2", &storedVal2, sizeof(storedVal2));

    return res1 + res2;
}

struct settings_handler config = {
    .name = "test", // name of the subtree
    .cprio = 0, // priority at which h_commit gets called
    .h_get = test_settings_get, // called to get data with settings_runtime_get(), could be usefull in bigger projects
    .h_set = test_settings_set, // called when data is loaded from storage with settings_load
    .h_commit = test_settings_commit, // called when data has fully been loaded
    .h_export = test_settings_export, // called to write all current settings with settings_save()
};

int main() {
    k_msleep(1000);

    int rc1 = settings_subsys_init();
    int rc2 = settings_register(&config);
    int rc3 = settings_load();

    if (rc1 || rc2 || rc3) {
        LOG_ERR("There was an error initializing settings subsystem");
    }

    for (int i = 0; i < 10; i++) {
        storedVal++;
        storedVal2--;

        // save one value
        int rc4 = settings_save_one("test/test_child", &storedVal, sizeof(storedVal));

        // save all values
        int rc5 = settings_save();

        LOG_INF("value 1 is %d, value 2 is %d", storedVal, storedVal2);

        if (rc4 || rc5) {
            LOG_ERR("There was an error saving data: %d, %d", rc4, rc5);
        }

        k_msleep(1000);
    }

    uint8_t fetched_val;

    settings_runtime_get("test/test_child", &fetched_val, sizeof(fetched_val));

    LOG_INF("Loaded value %d!", fetched_val);

    LOG_INF("Rebooting!");

    k_msleep(1000);

    sys_reboot(SYS_REBOOT_COLD);

    return 0;
}
