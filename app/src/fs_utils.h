/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FS_UTILS_H_
#define FS_UTILS_H_

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

int init_fs(struct fs_mount_t *mount_point);

int fs_wipe(uint32_t id);

int list_fs(const char * path);

#endif // FS_UTILS_H_
