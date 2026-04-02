#include <errno.h>

#include <f_core/device/openrocket_launch.h>
#include <openrocket_sensors.h>

int z_impl_ork_trigger_launch(void) {
#ifdef CONFIG_OPENROCKET_MANUAL_LAUNCH_TRIGGER
    return or_trigger_launch();
#else
    return -ENOTSUP;
#endif
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_ork_trigger_launch(void) { return z_impl_ork_trigger_launch(); }

#include <zephyr/syscalls/openrocket_launch_mrsh.c>
#endif
