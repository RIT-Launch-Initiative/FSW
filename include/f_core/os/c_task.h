/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef C_TASK_H
#define C_TASK_H

class CTask {
public:
    CTask(const char* name, int priority,  stack_size, uint64_t time_slice);

    bool AddTenant()

private:
};

#endif //C_TASK_H
