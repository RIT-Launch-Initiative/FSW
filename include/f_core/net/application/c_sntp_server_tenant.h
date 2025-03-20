#ifndef C_SNTP_SERVER_H
#define C_SNTP_SERVER_H

#include <zephyr/drivers/rtc.h>

#include "f_core/os/c_tenant.h"
#include "f_core/net/network/c_ipv4.h"
#include "f_core/net/transport/c_udp_socket.h"

class CSntpServerTenant : public CTenant {
public:
    enum SntpPrecisionExponents : int8_t {
        SNTP_NANOSECONDS_PRECISION = -30,
        SNTP_MICROSECONDS_PRECISION = -26,
        SNTP_MILLISECONDS_PRECISION = -23,
        SNTP_SECONDS_PRECISION = -20
    };

    static constexpr uint16_t SNTP_DEFAULT_PORT = 123;
    inline static CSntpServerTenant *instance = nullptr;

    /**
     * Singleton getter to avoid multiple instances of the SNTP server.
     */
    static CSntpServerTenant *getInstance(const CIPv4 &ipv4, uint16_t port = SNTP_DEFAULT_PORT) {
        if (instance == nullptr) {
            instance = new CSntpServerTenant(ipv4, port);
        }
        return instance;
    }

    /**
     * See parent docs
     */
    void Startup() override;

    /**
     * See parent docs
     */
    void Cleanup() override;

    /**
     * See parent docs
     */
    void Run() override;

private:
    static constexpr uint8_t SERVER_VERSION_NUMBER = 4;

    // Bit offsets for the li_vn_mode field
    static constexpr uint8_t LEAP_INDICATOR_BIT_OFFSET = 6;
    static constexpr uint8_t VERSION_NUMBER_BIT_OFFSET = 3;

    CUdpSocket sock; // The socket bound to port 123 (or specified port)
    CIPv4 ip;
    const device &rtcDevice;
    const uint8_t stratum;
    const uint8_t pollInterval;
    const int8_t precisionExponent;
    rtc_time *lastUpdatedTime; // Somewhat messy, but Zephyr API doesn't track last rtc_settime


    enum LeapIndicator : uint8_t {
        NO_WARNING = 0,
        LAST_MINUTE_HAS_61_SECONDS = 1,
        LAST_MINUTE_HAS_59_SECONDS = 2,
        ALARM_CONDITION = 3
    };

    enum Mode : uint8_t {
        RESERVED = 0,
        SYMMETRIC_ACTIVE = 1,
        SYMMETRIC_PASSIVE = 2,
        CLIENT = 3,
        SERVER = 4,
        BROADCAST = 5,
        RESERVED_FOR_NTP_CONTROL_MESSAGE = 6,
        RESERVED_FOR_PRIVATE_USE = 7
    };

    static constexpr uint32_t GPS_REFERENCE_CODE = 0x47505300; // "GPS\0"

    /**
     * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9  0  1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |LI | VN  |Mode |    Stratum    |     Poll      |   Precision    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          Root  Delay                           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Root  Dispersion                         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     Reference Identifier                       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Reference Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Originate Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Receive Timestamp (64)                     |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Transmit Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                 Key Identifier (optional) (32)                 |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                                                                |
      |                 Message Digest (optional) (128)                |
      |                                                                |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    struct SntpPacket {
        uint8_t li_vn_mode; // 2 + 3 + 3 bits
        uint8_t stratum;
        uint8_t poll;
        int8_t precision;
        float root_delay;
        float root_dispersion;
        uint32_t reference_id;
        uint64_t reference_timestamp;
        uint64_t originate_timestamp;
        uint64_t receive_timestamp;
        uint64_t transmit_timestamp;
    } __attribute__((packed));

    CSntpServerTenant(const CIPv4 &ipv4, uint16_t port = SNTP_DEFAULT_PORT)
        : CTenant("SNTP server"), sock(ipv4, port, port), ip(ipv4) {};

};

#endif // C_SNTP_SERVER_H