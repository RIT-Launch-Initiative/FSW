#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_REGISTER(main);

#define DEFAULT_VAL 0

static uint8_t storedVal = DEFAULT_VAL;

// read test setting from flash
static int test_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    int rc;

    LOG_INF("name is %s, len %d", name, len);

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

    return -ENOENT;
}

static int test_settings_export(int (*storage_func)(const char *name, const void *value, size_t val_len)) {
    return storage_func("test/test_child", &storedVal, sizeof(storedVal));
}

struct settings_handler config = {.name = "test", .h_set = test_settings_set, .h_export = test_settings_export};

int main() {
    k_msleep(1000);

    int rc1 = settings_subsys_init();
    int rc2 = settings_register(&config);
    int rc3 = settings_load();

    for (int i = 0; 1; i++) {
      storedVal++;
      int rc4 = settings_save_one("test/test_child", &storedVal, sizeof(storedVal));

      LOG_INF("HI, value is %d", storedVal);
      LOG_INF("RC1 = %d", rc1);
      LOG_INF("RC2 = %d", rc2);
      LOG_INF("RC3 = %d", rc3);
      LOG_INF("RC4 = %d", rc4);

      k_msleep(1000);
    }

    LOG_INF("Rebooting!");

    k_msleep(1000);

    sys_reboot(SYS_REBOOT_COLD);

    return 0;
}
