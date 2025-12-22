#include <f_core/radio/c_lora_tenant.h>

LOG_MODULE_REGISTER(CLoraTenant);

CLoraTenant::CLoraTenant(CLoraLink& radioLink, CMessagePort<LaunchLoraFrame>& txPort) : CRunnableTenant("CLoraTenant"),
    link(radioLink), router(radioLink),
    loraTransmitPort(txPort) {}

void CLoraTenant::Startup() {
}

void CLoraTenant::PadRun() {
    serviceTx();
    serviceRx(K_SECONDS(3));
}

void CLoraTenant::FlightRun() {
    serviceTx();
}

void CLoraTenant::LandedRun() {
    serviceTx();
    serviceRx(K_MSEC(100));
}

void CLoraTenant::Run() {
    switch (state) {
        case State::PAD:
            PadRun();
            break;
        case State::FLIGHT:
            FlightRun();
            break;
        case State::LANDED:
            LandedRun();
            break;
        default:
            LOG_ERR("Unknown state in CLoraTenant");
            break;
    }
}

void CLoraTenant::serviceTx() {
    LaunchLoraFrame msg{};
    int ret = loraTransmitPort.Receive(msg, K_NO_WAIT);
    if (ret < 0) {
        return;
    }

    LOG_INF("Sending LoRa broadcast on port %d, size %d", msg.Port, msg.Size);
    ret = link.Send(msg);
    if (ret < 0) {
        LOG_ERR("TX broadcast failed (%d)", ret);
    }
}

void CLoraTenant::serviceRx(const k_timeout_t timeout) {
    router.PollOnce(timeout);
}