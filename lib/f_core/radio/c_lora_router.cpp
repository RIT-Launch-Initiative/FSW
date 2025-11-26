
#include "f_core/radio/c_lora_router.h"

#include <cstdint>

#include <f_core/utils/c_hashmap.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraRouter);

void CLoraRouter::RegisterHandler(const uint16_t port, CLoraFrameHandler& handler) {
    if (!handlers.Insert(port, &handler)) {
        LOG_ERR("RadioRouter: failed to register handler for port %u", port);
    }
}

void CLoraRouter::PollOnce(const k_timeout_t timeout) {
    LaunchLoraFrame frame{};
    const int len = link.Receive(frame, timeout);
    if (len <= 0) {
        return;
    }

    CLoraFrameHandler* handler = handlers.Get(frame.Port).value_or(nullptr);

    if (handler != nullptr) {
        handler->HandleFrame(frame);
    } else {
        LOG_WRN("RadioRouter: no handler for port %u", frame.Port);
    }
}

