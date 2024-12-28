/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <f_core/os/c_tenant.h>
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/messaging/c_msgq_message_port.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

static k_poll_signal signal;

class CProducer : public CTenant {
public:
    CProducer(const char* name, const CMessagePort<int64_t>& messagePort, int index) : CTenant(name), messagePort(messagePort) {}

    void Run() override {
        messagePort.Send(k_uptime_get());
    }

private:
    CMessagePort<int64_t>& messagePort;
};

class CConsumer : public CTenant {
public:
    CConsumer(const char* name, const CMessagePort<int64_t>& messagePort, int64_t deltas[], size_t deltaSize, int index) :
        CTenant(name), messagePort(messagePort), deltas(deltas), deltaSize(deltaSize), pollIndex(index) {}

    void Run() override {
        if (deltaIndex >= deltaSize) {
            // Get signal value
            int sigResult = 0;
            k_poll_signal_check(&signal, nullptr, &sigResult);
            sigResult |= 1 << pollIndex;
            k_poll_signal_raise(&signal, sigResult);
            return;
        }

        int64_t delta;
        if (messagePort.Receive(delta)) {
            deltas[deltaIndex++] = delta;
        }
    }

private:
    CMessagePort<int64_t>& messagePort;
    int64_t* deltas;
    size_t deltaIndex = 0;
    size_t deltaSize;
    int pollIndex;
};

static constexpr size_t MAX_MESSAGE_QUEUE_SIZE = 100;
static constexpr size_t MAX_CONSUMER_COUNT = 10;

static int64_t MESSAGE_QUEUE_BUFFER[MAX_MESSAGE_QUEUE_SIZE * sizeof(int64_t)];
static k_msgq MESSAGE_QUEUES[MAX_CONSUMER_COUNT];

static void setup(CMessagePort<int64_t> messagePorts[], size_t messagePortCount, CProducer& producer, CConsumer consumers[], size_t consumerCount) {
    for (size_t i = 0; i < messagePortCount; i++) {
        messagePorts[i] = CMsgqMessagePort<int64_t>();
    }

    for (size_t i = 0; i < consumerCount; i++) {
        consumers[i] = CConsumer("Consumer", messagePorts[i], new int64_t[100], 100, i);
    }
}

static void teardown() {

}

static void benchmarkMsgq(int consumerCount = 1) {
}

int main() {
    k_poll_signal_init(&signal);




    return 0;
}
