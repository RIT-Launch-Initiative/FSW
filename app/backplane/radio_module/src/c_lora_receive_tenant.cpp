#include "c_lora_receive_tenant.h"
#include "c_radio_module.h"

#include <n_autocoder_network_defs.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraReceiveTenant);

static bool camerasTurnedOff = false;
void shutoffTimerExpirationFn(k_timer* timer) {
    LOG_INF("Shutting off cameras");

    CGpio *gpios = static_cast<CGpio*>(k_timer_user_data_get(timer));

    for (int i = 0; i < 4; i++)  {
        gpios[i].SetPin(0);
    }

    camerasTurnedOff = true;
}

void CLoraReceiveTenant::Startup() {}

void CLoraReceiveTenant::PostStartup() {}

void CLoraReceiveTenant::Run() {
    SetBoostDetected(NStateMachineGlobals::boostDetected);
    SetLandingDetected(NStateMachineGlobals::landingDetected);
    Clock();
}

void CLoraReceiveTenant::PadRun() {
    int port = 0;
    uint8_t buffer[255] = {0};

    int rxSize = receive(buffer, 255, &port);
    if (rxSize <= 0) {
        return;
    }

    if (rxSize > 2) {
        if (port == NNetworkDefs::RADIO_MODULE_COMMAND_PORT) { // Command
            // Apply commands to pins
            for (int i = 0; i < 4; i++)  {
                LOG_INF("Set GPIO %d to %d", i, (buffer[2] >> i) & 1);
                gpios[i].SetPin((buffer[2] >> i) & 1);
            }

            // Pack status into LoRaBroadcastData
            NTypes::LoRaBroadcastData pinStatus = {0};

            pinStatus.Port = NNetworkDefs::RADIO_MODULE_COMMAND_RESPONSE_PORT;
            pinStatus.Size = rxSize;

            // Get status of pins
            pinStatus.Payload[0] |= gpios[0].GetPin();
            pinStatus.Payload[0] |= gpios[1].GetPin() << 1;
            pinStatus.Payload[0] |= gpios[2].GetPin() << 2;
            pinStatus.Payload[0] |= gpios[3].GetPin() << 3;

            // Retransmit status so GS can verify
            if (loraTransmitTenant.transmit(pinStatus)) {
                LOG_ERR("Failed to transmit pin status");
            }
        } else if (port == NNetworkDefs::RADIO_MODULE_DATA_REQUEST_PORT) { // Data Request
            constexpr int bytesPerPort = 2;
            for (int i = 2; i < rxSize; i += bytesPerPort) {
                uint16_t parsedPort = buffer[i] << 8 | buffer[i + 1];
                if (loraTransmitTenant.padDataRequestedMap.Set(parsedPort, true)) {
                    LOG_INF("Requested data for port %d", parsedPort);
                } else {
                    LOG_ERR("No key for port %d", parsedPort);
                }
            }
        } else {
            LOG_INF("Sending LoRa received data to UDP %d", port);
            udp.SetDstPort(port);
            udp.TransmitAsynchronous(&buffer[2], rxSize - portOffset);
        }
    }
}

void CLoraReceiveTenant::FlightRun() {
    // Should not receive anything while in flight
}



void CLoraReceiveTenant::LandedRun() {
    if (!shutoffTimer.IsRunning() && !camerasTurnedOff) {
        shutoffTimer.SetUserData(gpios);
        shutoffTimer.StartTimer(1000 * 60 * 10); // 10 minutes
    }

    if (shutoffTimer.IsRunning() && camerasTurnedOff) {
        shutoffTimer.StopTimer();
    }
}

int CLoraReceiveTenant::receive(uint8_t* buffer, const int buffSize, int* port) const {
    const int size = loraTransmitTenant.lora.ReceiveSynchronous(buffer, buffSize, nullptr, nullptr, K_SECONDS(3));
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
    LOG_INF("Got data for port %d from LoRa", *port);
    return size;
}
