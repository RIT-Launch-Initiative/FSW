/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <array>
#include <string>
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/os/c_tenant.h>
#include <zephyr/logging/log.h>
#include <memory>


LOG_MODULE_REGISTER(main);

static int counter = 0;

class CTimeHogTenant : public CTenant {
public:
    CTimeHogTenant()
        : CTenant(("TimeHogTenant " + std::to_string(counter++)).data()), id(counter) {}

    void Startup() override {};

    void PostStartup() override {};

    void Run() override {
        uint32_t startTime = k_uptime_get_32();
        for (int32_t i = 0; i < max; i++) {
            long long randomMath = max / .1f;
            (void) randomMath;
        }

        LOG_INF("TimeHogTenant %d finished in %d ms", id, k_uptime_get_32() - startTime);
    };

private:
    static constexpr int32_t max = 0x7FFFFFFF;
    const int id;
};


int main() {
    static std::array<std::unique_ptr<CTenant>, 5> hogs;
    static std::array<std::unique_ptr<CTask>, 5> tasks;

    for (int i = 0; i < 5; i++) {
        tasks[i] = std::make_unique<CTask>(("Task " + std::to_string(i)).c_str(), 15, 512, 0);
        hogs[i] = std::make_unique<CTimeHogTenant>();

        hogs[i]->Startup();
        tasks[i]->AddTenant(*hogs[i]);
        tasks[i]->Initialize();
    }

    NRtos::StartRtos();

    return 0;
}
