#pragma once

#include <zephyr/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Trigger the OpenRocket simulation launch in kernel space.
 *
 * This is exposed as a syscall so application/user code does not call the
 * OpenRocket driver implementation directly.
 *
 * @retval 0 Launch trigger accepted
 * @retval -EALREADY Launch was already triggered
 * @retval -ENOTSUP Manual launch triggering is not enabled
 */
__syscall int ork_trigger_launch(void);

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/openrocket_launch.h>
