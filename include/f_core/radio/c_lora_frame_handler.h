#ifndef C_LORA_FRAME_HANDLER_H
#define C_LORA_FRAME_HANDLER_H

#include "f_core/radio/c_lora_link.h"

/**
 * Interface for components that want to handle frames on a particular port.
 */
class CLoraFrameHandler {
public:
    virtual ~CLoraFrameHandler() = default;

    /**
     * Handle an incoming radio frame.
     * @param frame The received radio frame
     */
    virtual void HandleFrame(const LaunchLoraFrame& frame) = 0;
};

#endif //C_LORA_FRAME_HANDLER_H