#include "c_lora_receive_tenant.h"
#include "c_radio_module.h"

#include <n_autocoder_network_defs.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraReceiveTenant);

void CLoraReceiveTenant::CLoraReceiveTenant::Startup() {}

void CLoraReceiveTenant::CLoraReceiveTenant::PostStartup() {}

void CLoraReceiveTenant::CLoraReceiveTenant::Run() {
}

void CLoraReceiveTenant::PadRun() {
    int port = 0;
    uint8_t buffer[255] = {0};

    int rxSize = receive(buffer, sizeof(buffer), &port);
    if (rxSize <= 0) {
        return;
    }

    if (rxSize > 2) {
        if (port == NNetworkDefs::RADIO_MODULE_COMMAND_PORT) { // Command
            LOG_INF("Command: 0x%x", buffer[2]);
            // Apply commands to pinsconst
            int result = gpios[0].SetPin(buffer[2] & 1);
            LOG_INF("Set Radiomod pin 0 with return code %d", result);
            result = gpios[1].SetPin((buffer[2] & (1 << 1)) >> 1);
            LOG_INF("Set Radiomod pin 1 with return code %d", result);
            result = gpios[2].SetPin((buffer[2] & (1 << 2)) >> 2);
            LOG_INF("Set Radiomod pin 2 with return code %d", result);
            result = gpios[3].SetPin((buffer[2] & (1 << 3)) >> 3);
            LOG_INF("Set Radiomod pin 3 with return code %d", result);

            // Pack status into RadioBroadcastData
            NTypes::RadioBroadcastData pinStatus = {0};

            pinStatus.port = NNetworkDefs::RADIO_MODULE_COMMAND_RESPONSE_PORT;
            pinStatus.size = rxSize;

            // Get status of pins
            pinStatus.data[0] |= gpios[0].GetPin();
            pinStatus.data[0] |= gpios[1].GetPin() << 1;
            pinStatus.data[0] |= gpios[2].GetPin() << 2;
            pinStatus.data[0] |= gpios[3].GetPin() << 3;

            // Retransmit status so GS can verify
            // TODO: Figure out why this blocks lora task from continuing
            // loraTransmitPort.Send(pinStatus);
        } else {
            udp.SetDstPort(port);
            udp.TransmitAsynchronous(&buffer[2], rxSize - portOffset);
        }
    }
}

void CLoraReceiveTenant::FlightRun() {
    // Should not receive anything while in flight
}

void CLoraReceiveTenant::LandedRun() {
    // Should not receive anything while landed
}

void CLoraReceiveTenant::GroundRun() {
    int port = 0;
    uint8_t buffer[255] = {0};

    int rxSize = receive(buffer, sizeof(buffer), &port);
    if (rxSize <= 0) {
        return;
    }

    udp.SetDstPort(port);
    udp.TransmitAsynchronous(buffer, rxSize);
}

int CLoraReceiveTenant::receive(const uint8_t* buffer, const int buffSize, int* port) {
    const int size = lora.ReceiveSynchronous(&buffer, buffSize, nullptr, nullptr);
    if (size == -EAGAIN) {
        return size;
    }

    if (size < 0) {
        LOG_ERR("Failed to receive over LoRa (%d)", size);
        return size;
    }


    if (size == 0) {
        LOG_WRN("Got 0 bytes from LoRa");
        return size;
    }

    *port = buffer[1] << 8 | buffer[0];
    LOG_DBG("Received %d bytes from LoRa for port %d", size, *port);
    return size;
}
