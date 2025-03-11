#include "f_core/net/application/c_udp_alert_tenant.h"

LOG_MODULE_REGISTER(CUdpAlertTenant);

void CUdpAlertTenant::Run() {
    std::array<uint8_t, MAGIC_BYTE_SIGNATURE_SIZE> buff{};
    if (sock.ReceiveAsynchronous(buff.data(), MAGIC_BYTE_SIGNATURE_SIZE)) {
        for (size_t i = 0; i < MAGIC_BYTE_SIGNATURE_SIZE; i++) {
            if (buff[i] != MAGIC_BYTE_SIGNATURE[i]) {
                return;
            }
        }

        AlertType alertType = static_cast<AlertType>(buff[MAGIC_BYTE_SIGNATURE_SIZE]);
        LOG_DBG("Received alert type %c", alertType); // Would have to be changed to %d if alerts go past 8 bits
    }
}
