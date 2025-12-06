#include <f_core/radio/c_lora_tenant.h>

LOG_MODULE_REGISTER(CLoraTenant);

CLoraTenant::CLoraTenant(CLoraLink& radioLink, CMessagePort<LaunchLoraFrame>& txPort) : CRunnableTenant("CLoraTenant"),
    link(radioLink), router(CLoraRouter(radioLink)),
    loraTransmitPort(txPort) {}

void CLoraTenant::Startup() {
}

void CLoraTenant::PadRun() {
    serviceTx();
    serviceRx(K_MSEC(50));
}

void CLoraTenant::FlightRun() {
    serviceTx();
}

void CLoraTenant::LandedRun() {
    serviceTx();
    serviceRx(K_MSEC(100));
}

void CLoraTenant::serviceTx() {
    LaunchLoraFrame msg{};
    int ret = loraTransmitPort.Receive(msg, K_NO_WAIT);
    if (ret < 0) {
        return;
    }

    ret = link.Send(msg.Port, msg.Payload, msg.Size);
    if (ret < 0) {
        LOG_ERR("TX broadcast failed (%d)", ret);
    }
}

void CLoraTenant::serviceRx(const k_timeout_t timeout) {
    router.PollOnce(timeout);
}