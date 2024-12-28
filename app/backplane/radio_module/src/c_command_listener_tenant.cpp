#include "c_command_listener_tenant.h"

void CCommandListenerTenant::Startup() {}

void CCommandListenerTenant::PostStartup() {
}

void CCommandListenerTenant::Run() {
    NTypes::RadioCommandBytes radioCommandBytes{0};
    udp.ReceiveSynchronous(&radioCommandBytes.data, sizeof(radioCommandBytes.size));

    for (int i = 0; i < 4; i++) {
        gpios[i].pin_set(radioCommandBytes.data[i]);
    }
}