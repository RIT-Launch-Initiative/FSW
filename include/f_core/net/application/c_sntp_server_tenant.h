#ifndef C_SNTP_SERVER_H
#define C_SNTP_SERVER_H

#include <zephyr/drivers/rtc.h>

#include "f_core/os/c_tenant.h"
#include "f_core/net/network/c_ipv4.h"
#include "f_core/net/transport/c_udp_socket.h"

// Static instances
// Note these must be defined outside the class to avoid linker issues
static rtc_time lastUpdatedTime;
static K_MUTEX_DEFINE(lastUpdatedTimeLock); // Use macro here so we dont need to call init

class CSntpServerTenant : public CTenant {
public:
    enum SntpPrecisionExponents : int8_t {
        SNTP_NANOSECONDS_PRECISION = -30,
        SNTP_MICROSECONDS_PRECISION = -26,
        SNTP_MILLISECONDS_PRECISION = -23,
        SNTP_SECONDS_PRECISION = -20
    };

    static constexpr uint16_t SNTP_DEFAULT_PORT = 123;

    /**
     * Singleton getter to avoid multiple instances of the SNTP server.
     */
    static CSntpServerTenant* GetInstance(const device& rtc, const CIPv4& ipv4, uint16_t port = SNTP_DEFAULT_PORT,
                                          uint8_t stratum = 1,
                                          uint8_t pollInterval = 4,
                                          int8_t precisionExponent = SNTP_NANOSECONDS_PRECISION) {
        if (instance == nullptr) {
            instance = new CSntpServerTenant(rtc, ipv4, port, stratum, pollInterval, precisionExponent);
        }
        return instance;
    }

    /**
     * Set the last updated time for the SNTP server
     * @param time[in] Time to set for last updated
     * @param timeout[in] Timeout for the mutex lock
     * @return 0 on success, -1 on failure to acquire lock in time
     */
    static int SetLastUpdatedTime(const rtc_time& time, const k_timeout_t timeout = K_MSEC(10)) {
        if (k_mutex_lock(&lastUpdatedTimeLock, timeout) != 0) {
            return -1;
        }

        lastUpdatedTime = time;
        k_mutex_unlock(&lastUpdatedTimeLock);
        return 0;
    }

    /**
     * Gets the last updated time for the SNTP server
     * @param time[out] Object to store the result in
     * @param timeout[in] Timeout for mutex lock
     * @return 0 on success, -1 on failure to acquire lock in time
     */
    static int GetLastUpdatedTime(rtc_time& time, const k_timeout_t timeout = K_MSEC(10)) {
        if (k_mutex_lock(&lastUpdatedTimeLock, timeout) != 0) {
            return -1;
        }

        time = lastUpdatedTime;
        k_mutex_unlock(&lastUpdatedTimeLock);
        return 0;
    }

    /**
     * See parent docs
     */
    void Startup() override;

    /**
     * See parent docs
     */
    void PostStartup() override;

    /**
     * See parent docs
     */
    void Cleanup() override;

    /**
     * See parent docs
     */
    void Run() override;

private:
    inline static CSntpServerTenant* instance = nullptr;

    static constexpr uint8_t SERVER_VERSION_NUMBER = 4;

    // Bit offsets for the li_vn_mode field
    static constexpr uint8_t LEAP_INDICATOR_BIT_OFFSET = 6;
    static constexpr uint8_t VERSION_NUMBER_BIT_OFFSET = 3;

    // Fixed point representation of 0.5 ms
    static constexpr uint32_t GNSS_ROOT_DISPERSION_FIXED_POINT = static_cast<uint32_t>(0.0005f * 65536);


    CUdpSocket sock; // The socket bound to port 123 (or specified port)
    CIPv4 ip;
    const device& rtcDevice;
    const uint8_t stratum;
    const uint8_t pollInterval;
    const int8_t precisionExponent;
    const uint16_t sockPort;


    enum LeapIndicator : uint8_t {
        LI_NO_WARNING = 0,
        LI_LAST_MINUTE_HAS_61_SECONDS = 1,
        LI_LAST_MINUTE_HAS_59_SECONDS = 2,
        LI_ALARM_CONDITION = 3
    };

    enum Mode : uint8_t {
        MODE_RESERVED = 0,
        MODE_SYMMETRIC_ACTIVE = 1,
        MODE_SYMMETRIC_PASSIVE = 2,
        MODE_CLIENT = 3,
        MODE_SERVER = 4,
        MODE_BROADCAST = 5,
        MODE_RESERVED_FOR_NTP_CONTROL_MESSAGE = 6,
        MODE_RESERVED_FOR_PRIVATE_USE = 7
    };

    struct SntpPacket {
#if defined(CONFIG_LITTLE_ENDIAN)
        uint8_t mode : 3;
        uint8_t vn : 3;
        uint8_t li : 2;
#else
        uint8_t li: 2;
        uint8_t vn: 3;
        uint8_t mode: 3;
#endif /* CONFIG_LITTLE_ENDIAN */
        uint8_t stratum;
        uint8_t poll;
        int8_t precision;
        uint32_t rootDelay;
        uint32_t rootDispersion;
        char referenceId[4] = {0};
        uint32_t refTimestampSeconds;
        uint32_t refTimestampFraction;
        uint32_t originateTimestampSeconds;
        uint32_t originateTimestampFraction;
        uint32_t rxTimestampSeconds;
        uint32_t rxTimestampFraction;
        uint32_t txTimestampSeconds;
        uint32_t txTimestampFraction;
    } __packed;

    static constexpr char GPS_REFERENCE_CODE[] = "GPS";


    CSntpServerTenant(const device& rtc, const CIPv4& ipv4, uint16_t port = SNTP_DEFAULT_PORT, uint8_t stratum = 1,
                      uint8_t pollInterval = 4, int8_t precisionExponent = SNTP_NANOSECONDS_PRECISION)
        : CTenant("SNTP server"), sock(ipv4, port, port), ip(ipv4), rtcDevice(rtc), stratum(stratum),
          pollInterval(pollInterval), precisionExponent(precisionExponent), sockPort(port) {}

    int getRtcTimeAsSeconds(uint32_t& seconds, uint32_t& nanoseconds) const;

    int getLastUpdateTimeAsSeconds(uint32_t& seconds, uint32_t& nanoseconds);
};

#endif // C_SNTP_SERVER_H
