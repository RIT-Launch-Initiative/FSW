#include "f_core/net/application/c_sntp_server_tenant.h"

#include <zephyr/drivers/rtc.h>

LOG_MODULE_REGISTER(CSntpServerTenant);

void CSntpServerTenant::Startup() {}

void CSntpServerTenant::Cleanup() {}

void CSntpServerTenant::Run() {
    SntpPacket clientPacket = {0};
    rtc_time time{0};
    uint32_t rxPacketSecondsTimestamp = 0;
    uint32_t rxPacketNanosecondsTimestamp = 0;
    uint32_t txPacketSecondsTimestamp = 0;
    uint32_t txPacketNanosecondsTimestamp = 0;
    uint32_t lastUpdateTimeSeconds = 0;
    uint32_t lastUpdateTimeNanoseconds = 0;

    // TODO: Merge TFTP code with the new UDP socket code
    int rxLen = sock.ReceiveAsynchronous(&clientPacket, sizeof(clientPacket));
    if (getRtcTimeAsSeconds(rxPacketSecondsTimestamp, rxPacketNanosecondsTimestamp) != 0) {
        return;
    }

    if (rxLen == 0) {
        return;
    } else if (rxLen < 0) {
        LOG_ERR("Failed to receive packet (%d)", rxLen);
        return;
    }

    if (clientPacket.mode != MODE_CLIENT) {
        LOG_INF("Received SNTP packet that was not from a client", clientPacket.mode);
        return;
    }

    uint8_t li = LI_NO_WARNING;
    if (getRtcTimeAsSeconds(txPacketSecondsTimestamp, txPacketNanosecondsTimestamp) |
        getLastUpdateTime(lastUpdateTimeSeconds, lastUpdateTimeNanoseconds) != 0) {
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
        .rootDelay = 0,                    // Unknown, but very small with the assumption Ethernet is used
        .rootDispersion = GNSS_ROOT_DISPERSION_FIXED_POINT,          // 0.5 ms
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

    // TODO: Direct this thang
    int ret = sock.TransmitAsynchronous(&packet, sizeof(packet));
    if (ret < 0) {
        LOG_ERR("Failed to transmit packet (%d)", ret);
    }
}


int CSntpServerTenant::getRtcTimeAsSeconds(uint32_t& seconds, uint32_t& nanoseconds) {
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

    seconds = time.tm_sec + time.tm_min * 60 + time.tm_hour * 3600;
    nanoseconds = time.tm_nsec;
    return 0;
}

int CSntpServerTenant::getLastUpdateTime(uint32_t& seconds, uint32_t& nanoseconds) {
    if (lastUpdatedTime == nullptr) {
        LOG_ERR("lastUpdatedTime is null");
        return -ENODATA;
    }

    seconds = lastUpdatedTime->tm_sec + lastUpdatedTime->tm_min * 60 + lastUpdatedTime->tm_hour * 3600;
    nanoseconds = lastUpdatedTime->tm_nsec;

    return 0;
}