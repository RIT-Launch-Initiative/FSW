#include "../../../../include/f_core/radio/frame_handlers/c_lora_freq_request_tenant.h"

#include <cerrno>
#include <cstring>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraFreqChangeTenant);

static void ackTimeoutCallback(struct k_timer* timer) {
    LOG_WRN("LoRa frequency change ACK not received within timeout");
    auto* tenant = static_cast<CLoraFreqRequestTenant*>(k_timer_user_data_get(timer));
    tenant->RevertFrequency();
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
    if (frame.Port != commandUdpPort) {
        return;
    }

    LOG_INF("Received LoRa frequency change ACK on port %d", frame.Port);
    ackTimer.StopTimer();
}

void CLoraFreqRequestTenant::Run() {
    float freqMhz = 0.0f;
    if (!receiveCommand(freqMhz)) {
        return;
    }

    prevFreqMhz = lora.GetFrequencyMhz();
    if (freqMhz == prevFreqMhz) {
        LOG_INF("Received frequency %f MHz is the same as current frequency, no change needed", static_cast<double>(freqMhz));
        return;
    }

    // Clear pending downlinks to prioritize the frequency change command
    downlinkMessagePort.Clear();


    if (!sendFrequencyCommand(freqMhz)) {
        LOG_WRN("Failed to queue LoRa frequency change command");
        return;
    }

    if (lora.SetFrequency(freqMhz) != 0) {
        LOG_ERR("Local frequency change to %f MHz failed", static_cast<double>(freqMhz));
        return;
    }

    LOG_INF("Changed LoRa frequency to %f MHz, waiting for ACK...", static_cast<double>(freqMhz));
    ackTimer.StartTimer(rxTimeout);
}

bool CLoraFreqRequestTenant::receiveCommand(float& freqMhz) {
    const int rcvResult = udp.ReceiveAsynchronous(&freqMhz, sizeof(float));
    if (rcvResult != sizeof(float)) {
        if (rcvResult > 0) {
            LOG_WRN("Unexpected command size %d (expected %zu)", rcvResult, sizeof(float));
        }
        return false;
    }
    return true;
}

bool CLoraFreqRequestTenant::sendFrequencyCommand(const float freqMhz) {
    LaunchLoraFrame frame{};
    frame.Port = commandUdpPort;
    frame.Size = sizeof(float);
    memcpy(frame.Payload, &freqMhz, sizeof(float));

    const int ret = downlinkMessagePort.Send(frame, K_NO_WAIT);
    if (ret == -ENOMSG) {
        LOG_WRN_ONCE("Downlink queue full, clearing and retrying");
        downlinkMessagePort.Clear();
        return downlinkMessagePort.Send(frame, K_NO_WAIT) == 0;
    }
    return ret == 0;
}

void CLoraFreqRequestTenant::RevertFrequency() {
    if (prevFreqMhz == 0.0f) {
        LOG_WRN("No previous frequency to revert to");
        return;
    }

    if (lora.SetFrequency(prevFreqMhz) != 0) {
        LOG_ERR("Failed to revert to previous frequency %f MHz", static_cast<double>(prevFreqMhz));
        return;
    }

    LOG_INF("Reverted to previous frequency %f MHz", static_cast<double>(prevFreqMhz));
}

