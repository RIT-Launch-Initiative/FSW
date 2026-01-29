#pragma once

#include <zephyr/kernel.h>

#include <f_core/os/c_runnable_tenant.h>
#include <f_core/state_machine/c_pad_flight_landing_state_machine.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/radio/c_lora_link.h>
#include <f_core/radio/c_lora_router.h>

class CLoraTenant : public CRunnableTenant, public CPadFlightLandedStateMachine {
public:
    /**
     * @brief Constructor
     * @param lora Device to use for LoRa communication.
     * @param txPort Message port for outgoing LoRa frames.
     */
    CLoraTenant(CLora& lora, CMessagePort<LaunchLoraFrame>& txPort);

    /**
     * See parent docs
     */
    void Startup() override;

    /**
     * See parent docs
     */
    void PadRun() override;

    /**
     * See parent docs
     */
    void FlightRun() override;

    /**
     * See parent docs
     */
    void LandedRun() override;

    /**
     * See parent docs
     */
    void GroundRun() override;

    /**
     * See parent docs
     */
    void Run() override;

    /**
     * @brief Register a frame handler for a specific port
     * @param port Port to register handler for
     * @param handler Handler to register
     */
    void RegisterFrameHandler(uint16_t port, CLoraFrameHandler& handler) {
        router.RegisterHandler(port, handler);
    }

    /**
     * @brief Register a default frame handler for frames with no specific handler
     * @param handler Default handler to register
     */
    void RegisterDefaultFrameHandler(CLoraFrameHandler& handler) {
        router.RegisterDefaultHandler(handler);
    }

private:
    /**
     * @brief Service transmit queue
     */
    void serviceTx();

    /**
     * @brief Service receive queue
     * @param timeout Timeout for receiving data
     */
    void serviceRx(const k_timeout_t timeout);

    CLoraLink link;
    CLoraRouter router;

    CMessagePort<LaunchLoraFrame>& loraTransmitPort;
};

