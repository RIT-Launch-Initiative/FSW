#include <f_core/radio/c_lora_tenant.h>

LOG_MODULE_REGISTER(CLoraTenant);

CLoraTenant::CLoraTenant(CLoraLink& radioLink, CMessagePort<LaunchLoraFrame>& txPort) : CRunnableTenant("CLoraTenant"),
    link(radioLink), router(CLoraRouter(radioLink)),
    loraTransmitPort(txPort) {}

void CLoraTenant::Startup() {}