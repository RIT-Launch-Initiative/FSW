#include "f_core/radio/frame_handlers/c_lora_freq_request_tenant.h"

#include <cerrno>
#include <cstring>
#include <arpa/inet.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraFreqRequestTenant);

static void ackTimeoutCallback(struct k_timer* timer) {
    LOG_WRN("LoRa frequency change ACK not received within timeout");
    auto* tenant = static_cast<CLoraFreqRequestTenant*>(k_timer_user_data_get(timer));
    tenant->RequestRevertFrequency();
}


CLoraFreqRequestTenant::CLoraFreqRequestTenant(const char* ipStr,
                                             CLora& lora,
                                             const uint16_t commandUdpPort,
                                             CMessagePort<LaunchLoraFrame>& loraDownlinkPort,
                                             const k_timeout_t rxTimeout)
    : CRunnableTenant("LoRa Frequency Request Tenant"),
      lora(lora),
      udp(CUdpSocket{CIPv4(ipStr), commandUdpPort, commandUdpPort}),
      downlinkMessagePort(loraDownlinkPort),
      commandUdpPort(commandUdpPort),
      rxTimeout(rxTimeout),
      ackTimer(ackTimeoutCallback, nullptr) {
    ackTimer.SetUserData(this);
}

void CLoraFreqRequestTenant::HandleFrame(const LaunchLoraFrame& frame) {
    LOG_INF("Received LoRa frequency change ACK on port %d", frame.Port);
    ackTimer.StopTimer();
}

void CLoraFreqRequestTenant::Run() {
    // Executes if a revert frequency request is pending
    if (revertFrequencyRequested) {
        revertFrequencyRequested = false;
        revertFrequency();
        return;
    }

    // Executes after queuing a frequency change and finishing a Run cycle
    if (freqHzRequested != 0) {
        const float freqMhz = static_cast<float>(freqHzRequested) / 1'000'000.0f;
        if (lora.SetFrequency(freqHzRequested) != 0) {
            LOG_ERR("Local frequency change to %f MHz failed", static_cast<double>(freqMhz));
        } else {
            LOG_INF("Changed LoRa frequency to %f MHz, waiting for ACK", static_cast<double>(freqMhz));
            ackTimer.StartTimer(rxTimeout);
        }

        freqHzRequested = 0;
        return;
    }

    uint32_t freqHz = 0;
    if (!receiveCommand(freqHz)) {
        return;
    }

    prevFreqHz = lora.GetFrequencyHz();
    prevFreqMhz = static_cast<float>(prevFreqHz) / 1'000'000.0f;

    if (freqHz == prevFreqHz) {
        LOG_INF("Received frequency %f MHz is the same as current frequency, no change needed",
                static_cast<double>(prevFreqMhz));
        return;
    }

    // Clear pending downlinks to prioritize the frequency change command
    downlinkMessagePort.Clear();

    // Validate frequency range before sending command
    const bool within915 = (freqHz >= 902'000'000u && freqHz <= 928'000'000u);
    const bool within433 = (freqHz >= 410'000'000u && freqHz <= 525'000'000u);
    if (!within915 && !within433) {
        const float freqMhz = static_cast<float>(freqHz) / 1'000'000.0f;
        LOG_WRN("Requested frequency %f MHz is out of valid ranges (902-928 MHz or 410-525 MHz)",
                static_cast<double>(freqMhz));
        return;
    }

    if (!sendFrequencyCommand(freqHz)) {
        LOG_WRN("Failed to queue LoRa frequency change command");
        return;
    }

    freqHzRequested = freqHz;
}

void CLoraFreqRequestTenant::RequestRevertFrequency() {
    revertFrequencyRequested = true;
    ackTimer.StopTimer();
}

bool CLoraFreqRequestTenant::receiveCommand(uint32_t& freqHz) {
    uint32_t networkOrder = 0;

    const int rcv = udp.ReceiveAsynchronous(&networkOrder, sizeof(networkOrder));
    if (rcv != sizeof(networkOrder)) {
        if (rcv > 0) {
            LOG_WRN("Unexpected command size %d (expected %zu)", rcv, sizeof(networkOrder));
        }
        return false;
    }

    const uint32_t hostOrder = ntohl(networkOrder);
    float freqMhz = 0.0f;
    memcpy(&freqMhz, &hostOrder, sizeof(freqMhz));

    if (freqMhz <= 0.0f) {
        LOG_WRN("Received non-positive frequency %f MHz", static_cast<double>(freqMhz));
        return false;
    }

    freqHz = static_cast<uint32_t>(freqMhz * 1'000'000.0f);
    return true;
}

bool CLoraFreqRequestTenant::sendFrequencyCommand(const uint32_t freqHz) {
    const float freqMhz = static_cast<float>(freqHz) / 1'000'000.0f;

    LaunchLoraFrame frame{};
    frame.Port = commandUdpPort;
    frame.Size = sizeof(uint32_t);

    const uint32_t networkOrder = htonl(freqHz);
    memcpy(frame.Payload, &networkOrder, sizeof(networkOrder));

    const int ret = downlinkMessagePort.Send(frame, K_NO_WAIT);
    if (ret == -ENOMSG) {
        LOG_WRN_ONCE("Downlink queue full, clearing and retrying");
        downlinkMessagePort.Clear();
        return downlinkMessagePort.Send(frame, K_NO_WAIT) == 0;
    }
    if (ret != 0) {
        LOG_ERR("Failed to queue frequency command %f MHz (%d)", static_cast<double>(freqMhz), ret);
    }
    return ret == 0;
}

void CLoraFreqRequestTenant::revertFrequency() {
    if (prevFreqHz == 0) {
        LOG_WRN("No previous frequency to revert to");
        return;
    }

    if (lora.SetFrequency(prevFreqHz) != 0) {
        const float prevMhz = static_cast<float>(prevFreqHz) / 1'000'000.0f;
        LOG_ERR("Failed to revert to previous frequency %f MHz", static_cast<double>(prevMhz));
        return;
    }

    LOG_INF("Reverted to previous frequency %f MHz", static_cast<double>(prevFreqMhz));
}
