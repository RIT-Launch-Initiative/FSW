#pragma once

#include "f_core/radio/c_lora_link.h"

/**
 * Interface for components that want to handle frames on a particular port.
 */
class CLoraFrameHandler {
public:
    virtual ~CLoraFrameHandler() = default;

    /**
     * Handle an incoming radio frame.
     * @param rxFrame The received radio frame
     */
    virtual void HandleFrame(const ReceivedLaunchLoraFrame& rxFrame) = 0;
};

