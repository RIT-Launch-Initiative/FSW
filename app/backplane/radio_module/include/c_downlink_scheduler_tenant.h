#ifndef RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H
#define RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H

#include <f_core/radio/c_lora_frame_handler.h>

class CDownlinkSchedulerTenant : public CTenant, public CLoraFrameHandler {

};


#endif //RADIO_MODULE_C_DOWNLINK_SCHEDULER_TENANT_H