#include "c_lora_freq_change_tenant.h"

#include <cerrno>
#include <cstring>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraFreqChangeTenant);

CLoraFreqChangeTenant::CLoraFreqChangeTenant(const char* ipStr,
                                             CLora& lora,
                                             const uint16_t commandUdpPort,
                                             CMessagePort<LaunchLoraFrame>& loraDownlinkPort,
                                             const k_timeout_t rxTimeout)
    : CRunnableTenant(name),
      lora(lora),
      udp(CUdpSocket{CIPv4(ipStr), commandUdpPort, commandUdpPort}),
      downlinkMessagePort(loraDownlinkPort),
      commandUdpPort(commandUdpPort),
      rxTimeout(rxTimeout) {}

void CLoraFreqChangeTenant::Run() {
    float freqMhz = 0.0f;
    if (!receiveCommand(freqMhz)) {
        return;
    }

    // Clear pending downlinks to prioritize the frequency change command
    downlinkMessagePort.Clear();


    if (!sendFrequencyCommand(freqMhz)) {
        LOG_WRN("Failed to queue LoRa frequency change command");
        return;
    }

    prevFreqMhz = lora.GetFrequencyMhz();
    if (lora.SetFrequency(freqMhz) != 0) {
        LOG_ERR("Local frequency change to %f MHz failed", static_cast<double>(freqMhz));
        return;
    }

    // TODO: CSoftTimer + Callback
}

bool CLoraFreqChangeTenant::receiveCommand(float& freqMhz) {
    const int rcvResult = udp.ReceiveAsynchronous(&freqMhz, sizeof(float));
    if (rcvResult != sizeof(float)) {
        if (rcvResult > 0) {
            LOG_WRN("Unexpected command size %d (expected %zu)", rcvResult, sizeof(float));
        }
        return false;
    }
    return true;
}

bool CLoraFreqChangeTenant::sendFrequencyCommand(const float freqMhz) {
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


