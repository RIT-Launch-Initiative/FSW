#include "f_core/net/application/c_sntp_server_tenant.h"

#include <zephyr/drivers/rtc.h>

LOG_MODULE_REGISTER(CSntpServerTenant);

void CSntpServerTenant::Startup() {
    LOG_INF("Starting SNTP server on port %d", sockPort);
}
void CSntpServerTenant::PostStartup() {
    rtc_time time = {0};
    if (rtc_get_time(&rtcDevice, &time) == -ENODATA) {
        LOG_INF("Failed to get RTC time on SNTP server startup. Defaulting to 2025-01-01 00:00:00");
        // Default to 1970-01-01 00:00:00 until the RTC is set
        constexpr rtc_time tm = {
            .tm_sec = 0,
            .tm_min = 0,
            .tm_hour = 0,
            .tm_mday = 1,
            .tm_mon = 0,
            .tm_year = 2025 - 1900, // We can't use 1970 because settime fails on it for some reason? Should investigate
            .tm_wday = 4,
            .tm_yday = 0,
            .tm_isdst = -1,
            .tm_nsec = 0,
        };
        // TODO: Note there might be some weird Zephyr bug here to be investigated. Date is inaccurate and it might be because of the ordering of the struct?
        // Investigate this at a later time
        int ret = rtc_set_time(&rtcDevice, &tm);
        if (ret != 0) {
            LOG_ERR("Failed to set RTC time on SNTP server startup (%d)", ret);
            if (ret == -EINVAL) {
                LOG_ERR("EINVAL error setting RTC. Confirm tm struct is properly formatted");
            }
        }
    }
    SetLastUpdatedTime(time);
}

void CSntpServerTenant::Cleanup() {}

void CSntpServerTenant::Run() {
    SntpPacket clientPacket = {0};
    uint32_t rxPacketSecondsTimestamp = 0;
    uint32_t rxPacketNanosecondsTimestamp = 0;
    uint32_t txPacketSecondsTimestamp = 0;
    uint32_t txPacketNanosecondsTimestamp = 0;
    uint32_t lastUpdateTimeSeconds = 0;
    uint32_t lastUpdateTimeNanoseconds = 0;
    sockaddr srcAddr = {0};
    socklen_t srcAddrLen = sizeof(srcAddr);

    const int rxLen = sock.ReceiveAsynchronous(&clientPacket, sizeof(clientPacket), &srcAddr, &srcAddrLen);

    if (rxLen == 0) {
        return;
    } else if (rxLen < 0) {
        LOG_ERR("Failed to receive packet (%d)", rxLen);
        return;
    }

    if (getRtcTimeAsSeconds(rxPacketSecondsTimestamp, rxPacketNanosecondsTimestamp) != 0) {
        return;
    }

    if (clientPacket.mode != MODE_CLIENT) {
        LOG_INF("Received SNTP packet that was not from a client (%d)", clientPacket.mode);
        return;
    }

    uint8_t li = LI_NO_WARNING;
    if (getRtcTimeAsSeconds(txPacketSecondsTimestamp, txPacketNanosecondsTimestamp) ||
        getLastUpdateTimeAsSeconds(lastUpdateTimeSeconds, lastUpdateTimeNanoseconds)) {
        li = LI_ALARM_CONDITION;
        // Keep going. The packet will be sent with the alarm condition signaling we are desynchronized
    }

    SntpPacket packet = {
#ifdef CONFIG_LITTLE_ENDIAN // ISO C++ compliance :v
        .mode = MODE_SERVER,
        .vn = SERVER_VERSION_NUMBER,
        .li = li,
#else
        .li = li,
        .vn = SERVER_VERSION_NUMBER,
        .mode = MODE_SERVER,
#endif
        .stratum = stratum,
        .poll = pollInterval,
        .precision = precisionExponent,
        .rootDelay = 0, // Unknown, but very small with the assumption Ethernet is used
        .rootDispersion = GNSS_ROOT_DISPERSION_FIXED_POINT, // 0.5 ms
        .referenceId = GPS_REFERENCE_CODE, // Currently only GPS is the expected reference (stratum 1)
        .refTimestampSeconds = lastUpdateTimeSeconds,
        .refTimestampFraction = lastUpdateTimeNanoseconds,
        .originateTimestampSeconds = clientPacket.txTimestampSeconds,
        .originateTimestampFraction = clientPacket.txTimestampFraction,
        .rxTimestampSeconds = rxPacketSecondsTimestamp,
        .rxTimestampFraction = rxPacketNanosecondsTimestamp,
        .txTimestampSeconds = txPacketSecondsTimestamp,
        .txTimestampFraction = txPacketNanosecondsTimestamp,
    };

    sockaddr_in clientAddr = *reinterpret_cast<const sockaddr_in *>(&srcAddr);
    uint16_t clientPort = ntohs(clientAddr.sin_port);
    CUdpSocket respondSock(ip, sockPort, clientPort);

    int ret = respondSock.TransmitAsynchronous(&packet, sizeof(packet));
    if (ret < 0) {
        LOG_ERR("Failed to transmit packet (%d)", ret);
    }
}


int CSntpServerTenant::getRtcTimeAsSeconds(uint32_t& seconds, uint32_t& nanoseconds) const {
    rtc_time time;
    int ret = rtc_get_time(&rtcDevice, &time);
    if (ret != 0) {
        if (ret == -ENODATA) {
            LOG_ERR("RTC time not set");
        } else {
            LOG_ERR("RTC time get failed (%d)", ret);
        }

        return ret;
    }

    tm timeinfo = {
        .tm_sec = time.tm_sec,
        .tm_min = time.tm_min,
        .tm_hour = time.tm_hour,
        .tm_mday = time.tm_mday,
        .tm_mon = time.tm_mon - 1,
        .tm_year = time.tm_year,
    };
    seconds = mktime(&timeinfo);
    nanoseconds = time.tm_nsec;
    return 0;
}

int CSntpServerTenant::getLastUpdateTimeAsSeconds(uint32_t& seconds, uint32_t& nanoseconds) {
    rtc_time time{0};
    int ret = GetLastUpdatedTime(time, K_NO_WAIT);
    if (ret != 0) {
        return ret;
    }

    // Calculate seconds since epoch properly with all time components
    struct tm timeinfo = {
        .tm_sec = time.tm_sec,
        .tm_min = time.tm_min,
        .tm_hour = time.tm_hour,
        .tm_mday = time.tm_mday,
        .tm_mon = time.tm_mon - 1, // tm months are 0-11
        .tm_year = time.tm_year,   // already years since 1900
    };
    seconds = mktime(&timeinfo);
    nanoseconds = time.tm_nsec;
    return 0;
}
