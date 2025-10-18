#include "f_core/net/application/c_sntp_server_tenant.h"

#include "zephyr/net/socket_service.h"

#include <zephyr/net/net_ip.h>

LOG_MODULE_REGISTER(CSntpServerTenant);

extern net_socket_service_desc sntpSocketService;
extern "C" void sntpSocketServiceHandler(net_socket_service_event* pev) {
    auto userData = static_cast<CUdpSocket::SocketServiceUserData*>(pev->user_data);

    if (userData == nullptr) {
        LOG_ERR("User data is null in alertSocketServiceHandler");
        k_oops();
    }

    auto* tenant = static_cast<CCallbackTenant*>(userData->userData);
    if (tenant == nullptr) {
        LOG_ERR("Tenant is null in tftpSocketServiceHandler");
        k_oops();
    }

    tenant->Callback();
}

void CSntpServerTenant::Register() {
    LOG_INF("Starting SNTP server on port %d", sockPort);
    int ret = sock.RegisterSocketService(&sntpSocketService, this);
    if (ret < 0) {
        LOG_ERR("Failed to register socket service for CSntpServerTenant: %d", ret);
    }

    rtc_time time = {0};

    int ret = rtc.GetTime(time);
    if (ret < 0) {
        LOG_ERR("Failed to get RTC time on SNTP server startup (%d). Defaulting to 2025-01-01 00:00:00", ret);
        // Default to 2025-01-01 00:00:00 until the RTC is set
        rtc_time tm = {
            .tm_sec = 0,
            .tm_min = 0,
            .tm_hour = 0,
            .tm_mday = 1,
            .tm_mon = 0,
            // STM32 RTC is from 2000. This leads to some scuffed things. 100 will correspond to 1900, but 101 corresponds to 2001. See rtc_ll_stm32.c
            .tm_year = 125, // 2025 - 1900 = 125
            .tm_wday = 4,
            .tm_yday = 0,
            .tm_isdst = -1,
            .tm_nsec = 0,
        };

        ret = rtc.SetTime(tm);
        if (ret != 0) {
            LOG_ERR("Failed to set RTC time on SNTP server startup (%d)", ret);
            if (ret == -EINVAL) {
                LOG_ERR("EINVAL error setting RTC. Confirm tm struct is properly formatted");
            }
        }
    }

    int retryCount = 0;
    while (SetLastUpdatedTime(time) != 0 && retryCount < 5) {
        k_sleep(K_MSEC(100));
        retryCount++;
        LOG_ERR("Failed to set last updated time. Retrying (%d)", retryCount);
    }
}

void CSntpServerTenant::Cleanup() {}

void CSntpServerTenant::Callback() {
    SntpPacket clientPacket = {0};
    uint32_t rxPacketSecondsTimestamp = 0;
    uint32_t txPacketSecondsTimestamp = 0;
    uint32_t lastUpdateTimeSeconds = 0;
    sockaddr srcAddr = {0};
    socklen_t srcAddrLen = sizeof(srcAddr);

    const int rxLen = sock.ReceiveAsynchronous(&clientPacket, sizeof(clientPacket), &srcAddr, &srcAddrLen);

    if (rxLen == 0) {
        return;
    } else if (rxLen < 0) {
        LOG_ERR("Failed to receive packet (%d)", rxLen);
        return;
    }

    if (rtc.GetUnixTime(rxPacketSecondsTimestamp) != 0) {
        LOG_ERR("Failed to get unix time on SNTP server startup");
    }

    if (clientPacket.mode != MODE_CLIENT) {
        LOG_ERR("Received SNTP packet that was not from a client (%d)", clientPacket.mode);
        return;
    }

    uint8_t li = LI_NO_WARNING;
    if (rtc.GetUnixTime(txPacketSecondsTimestamp) ||
        getLastUpdateTimeAsSeconds(lastUpdateTimeSeconds)) {
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
        .rootDispersion = htonl(GNSS_ROOT_DISPERSION_FIXED_POINT), // 0.5 ms
        .refTimestampSeconds = reckonAndByteSwapTimestamp(lastUpdateTimeSeconds),
        .refTimestampFraction = 0,
        .originateTimestampSeconds = clientPacket.txTimestampSeconds,
        .originateTimestampFraction = clientPacket.txTimestampFraction,
        .rxTimestampSeconds = reckonAndByteSwapTimestamp(rxPacketSecondsTimestamp),
        .txTimestampSeconds = reckonAndByteSwapTimestamp(txPacketSecondsTimestamp),
    };

    // Currently only GPS is the expected reference (stratum 1)
    memcpy(packet.referenceId, GPS_REFERENCE_CODE, 4);

    sockaddr_in clientAddr = *reinterpret_cast<const sockaddr_in*>(&srcAddr);
    uint16_t clientPort = ntohs(clientAddr.sin_port);
    sock.SetDstPort(clientPort);

    int ret = sock.TransmitAsynchronous(&packet, sizeof(packet));
    if (ret < 0) {
        LOG_ERR("Failed to transmit packet (%d)", ret);
    }
}


int CSntpServerTenant::getLastUpdateTimeAsSeconds(uint32_t& seconds) {
    rtc_time time{0};
    if (int ret = GetLastUpdatedTime(time, K_NO_WAIT); ret != 0) {
        return ret;
    }

    // Calculate seconds since epoch properly with all time components
    struct tm timeinfo = {
        .tm_sec = time.tm_sec,
        .tm_min = time.tm_min,
        .tm_hour = time.tm_hour,
        .tm_mday = time.tm_mday,
        .tm_mon = time.tm_mon - 1,
        .tm_year = time.tm_year
    };
    seconds = mktime(&timeinfo);
    return 0;
}
