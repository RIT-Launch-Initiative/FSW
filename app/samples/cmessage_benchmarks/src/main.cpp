/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <array>
#include <zephyr/kernel.h>
#include <f_core/os/c_tenant.h>
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/messaging/c_msgq_message_port.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);


// TODO: Find a better way of doing this LOL
K_MSGQ_DEFINE(queueOne, sizeof(int64_t), 10, 4);
K_MSGQ_DEFINE(queueTwo, sizeof(int64_t), 10, 4);
K_MSGQ_DEFINE(queueThree, sizeof(int64_t), 10, 4);
K_MSGQ_DEFINE(queueFour, sizeof(int64_t), 10, 4);
K_MSGQ_DEFINE(queueFive, sizeof(int64_t), 10, 4);
K_MSGQ_DEFINE(queueSix, sizeof(int64_t), 10, 4);
K_MSGQ_DEFINE(queueSeven, sizeof(int64_t), 10, 4);
K_MSGQ_DEFINE(queueEight, sizeof(int64_t), 10, 4);
K_MSGQ_DEFINE(queueNine, sizeof(int64_t), 10, 4);
K_MSGQ_DEFINE(queueTen, sizeof(int64_t), 10, 4);
static constexpr std::array<k_msgq*, 10> queues = {&queueOne, &queueTwo, &queueThree, &queueFour, &queueFive, &queueSix, &queueSeven,
                                                   &queueEight, &queueNine, &queueTen};

static k_poll_signal signal;

class CProducer : public CTenant {
public:
    CProducer(const char* name, const CMessagePort<int64_t>& messagePort) : CTenant(name), messagePort(messagePort) {}

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

static void reportResults(const char* name, const int64_t deltas[], const size_t deltaSize) {

    uint64_t sum = 0;
    for (size_t i = 0; i < deltaSize; i++) {
        LOG_INF("Delta %d: %lld", i, deltas[i]);
        sum += deltas[i];
    }
    LOG_INF("Average: %lld", sum / deltaSize);
}

static void benchmarkMsgq(const int consumerCount = 1, const int deltaSize = 100) {
    static_assert(consumerCount > 0, "Must have at least one consumer");
    static_assert(consumerCount <= 10, "Cannot have more than 10 consumers");

    int64_t allDeltas[consumerCount][deltaSize] = {0};

    for (int i = 0; i < consumerCount; i++) {
        CMsgqMessagePort<int64_t> messagePort(*queues[i]);
        CConsumer consumer("Consumer", messagePort, allDeltas[i], 100, i);
    }

    for (int i = 0; i < deltaSize; i++) {
        CMsgqMessagePort<int64_t> messagePort(*queues[0]);
        CProducer producer("Producer", messagePort);
    }
}

int main() {
    k_poll_signal_init(&signal);




    return 0;
}
