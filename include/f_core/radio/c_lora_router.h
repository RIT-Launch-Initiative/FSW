#ifndef C_LORA_ROUTER_H
#define C_LORA_ROUTER_H

#include "f_core/radio/c_lora_link.h"
#include "f_core/radio/c_lora_frame_handler.h"

#include <f_core/utils/c_hashmap.h>
#include <zephyr/kernel.h>

class CLoraRouter {
public:
    explicit CLoraRouter(CLoraLink& link) : link(link) {}

    void RegisterHandler(uint16_t port, CLoraFrameHandler& handler);

    /**
     * Blocking poll once. If no frame is received before @timeout, returns without calling handlers.
     * @param timeout Timeout for receiving data
     */
    void PollOnce(const k_timeout_t timeout);

private:
    CLoraLink& link;
    CHashMap<uint16_t, CLoraFrameHandler*> handlers;
};

#endif // C_LORA_ROUTER_H
