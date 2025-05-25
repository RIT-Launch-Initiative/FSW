/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/os/c_task.h>
#include <f_core/os/c_tenant.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

static constexpr int maxCount = 10000;

class CIncrementor : public CTenant {
public:
    CIncrementor(const char *name, int &counter) : CTenant(name), counter(counter) {
    }

    void Run() override {
        counter++;
    }

private:
    static int id;
    int &counter;
    bool finished = false;
};

int main() {
    int counter = 0;
    static CIncrementor incrementerOne("One", counter);
    static CIncrementor incrementerTwo("Two", counter);
    static CIncrementor incrementerThree("Three", counter);

    static CTask taskOne("One", 15, 512, 0);
    static CTask taskTwo("Two", 15, 512, 0);
    static CTask taskThree("Three", 15, 512, 0);

    taskOne.AddTenant(incrementerOne);
    taskTwo.AddTenant(incrementerTwo);
    taskThree.AddTenant(incrementerThree);

    NRtos::AddTask(taskOne);
    NRtos::AddTask(taskTwo);
    NRtos::AddTask(taskThree);

    NRtos::StartRtos();
    k_msleep(5000); // More than enough time to finish counting to a "small" enough number (i.e. 10000)
    NRtos::StopRtos();

    LOG_INF("%d", counter);

    return 0;
}
