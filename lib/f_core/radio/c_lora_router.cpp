
#include "f_core/radio/c_lora_router.h"

#include <cstdint>

#include <f_core/utils/c_hashmap.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraRouter);

void CLoraRouter::RegisterHandler(const uint16_t port, CLoraFrameHandler& handler) {
    if (!handlers.Insert(port, &handler)) {
        LOG_ERR("failed to register handler for port %u", port);
    }
}

void CLoraRouter::PollOnce(const k_timeout_t timeout) {
    ReceivedLaunchLoraFrame rxFrame{};
    const int len = link.Receive(rxFrame, timeout);
    if (len < 0) {
        return;
    }
    LOG_INF("Received LoRa frame on port %u, size %d", frame.Port, len);

    CLoraFrameHandler* handler = handlers.Get(rxFrame.Frame.Port).value_or(defaultHandler);

    if (handler != nullptr) {
        handler->HandleFrame(rxFrame);
    } else {
        LOG_WRN("no handler for port %u", frame.Port);
    }
}

