#include <f_core/radio/c_lora_tenant.h>

LOG_MODULE_REGISTER(CLoraTenant);

CLoraTenant::CLoraTenant(CLora& lora, CMessagePort<LaunchLoraFrame>& txPort) : CRunnableTenant("CLoraTenant"),
    link(lora), router(link),
    loraTransmitPort(txPort) {}

void CLoraTenant::Startup() {

}

void CLoraTenant::PadRun() {
    serviceTx();
    serviceRx(K_MSEC(2000));
}

void CLoraTenant::FlightRun() {
    serviceTx();
}

void CLoraTenant::LandedRun() {
    serviceTx();
    serviceRx(K_MSEC(1000));
}

void CLoraTenant::GroundRun() {
    serviceTx();
    serviceRx(K_MSEC(1000));
}

void CLoraTenant::Run() {
    this->Clock();
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