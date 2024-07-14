#include <string.h>
#include <f_core/net/device/c_lora.h>


CLora::CLora(const device* lora_dev) : lora_dev(lora_dev) {}

CLora::CLora(const device* lora_dev, const lora_modem_config& config) : lora_dev(lora_dev) {
    memcpy(&this->config, &config, sizeof(lora_modem_config));
}



int CLora::TransmitSynchronous(const void* data, size_t len) {

    return 0;
}

int CLora::ReceiveSynchronous(void* data, size_t len) {
    return 0;
}

int CLora::TransmitAsynchronous(const void* data, size_t len) {
    return 0;
}

int CLora::ReceiveAsynchronous(void* data, size_t len) {
    return 0;
}

int CLora::SetRxTimeout(int timeout) {
    return 0;
}
