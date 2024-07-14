#include <f_core/net/device/c_lora.h>


CLora::CLora(const device* lora_dev) : lora_dev(lora_dev) {}


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
