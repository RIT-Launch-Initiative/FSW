#ifndef N_ALERTS_H
#define N_ALERTS_H
#include <array>
#include <stdint.h>

namespace NAlerts {
    // TODO: Autocoder?
    // 16 bits instead of 8 for potential future expansion (Although I hope we don't have to deal with >255 events!)
    typedef enum : uint16_t {
        BOOST = 'b',
        APOGEE = 'a',
        NOSEOVER = 'n',
        LANDED = 'l',
    } AlertType;

    // Magic byte signature, to limit the chance of randomness misfiring an alert
    static constexpr std::array<uint8_t, 6> MAGIC_BYTE_SIGNATURE = {'L', 'A', 'U', 'N', 'C', 'H'};
    static constexpr size_t MAGIC_BYTE_SIGNATURE_SIZE = sizeof(MAGIC_BYTE_SIGNATURE);
    static constexpr size_t ALERT_PACKET_SIZE = MAGIC_BYTE_SIGNATURE_SIZE + sizeof(AlertType);
}
#endif //N_ALERTS_H
