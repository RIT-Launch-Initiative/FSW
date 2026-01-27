#pragma once

#include "f_core/radio/c_lora_link.h"
#include "f_core/radio/c_lora_frame_handler.h"

#include <f_core/utils/c_hashmap.h>
#include <zephyr/kernel.h>

class CLoraRouter {
public:
    explicit CLoraRouter(CLoraLink& link) : link(link) {}

    /**
     * Register a frame handler for a specific port
     * @param port Port to register handler for
     * @param handler Handler to run when frame is received on @port
     */
    void RegisterHandler(uint16_t port, CLoraFrameHandler& handler);

    /**
     * Register a default frame handler for frames with no specific handler
     * @param handler Handler to run when no specific handler is registered for a received frame
     */
    void RegisterDefaultHandler(CLoraFrameHandler& handler) {
        defaultHandler = &handler;
    }

    /**
     * Blocking poll once. If no frame is received before @timeout, returns without calling handlers.
     * @param timeout Timeout for receiving data
     */
    void PollOnce(const k_timeout_t timeout);

private:
    CLoraLink& link;
    CHashMap<uint16_t, CLoraFrameHandler*> handlers;
    CLoraFrameHandler *defaultHandler = nullptr;
};

#endif // C_LORA_ROUTER_H
