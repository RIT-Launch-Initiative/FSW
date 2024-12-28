/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <array>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <f_core/os/c_tenant.h>
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/messaging/c_msgq_message_port.h>

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

static constexpr std::array<k_msgq*, 10> queues = {
    &queueOne, &queueTwo, &queueThree, &queueFour, &queueFive,
    &queueSix, &queueSeven, &queueEight, &queueNine, &queueTen
};

static k_poll_signal consumersFinishedSignal;

class CProducer : public CTenant {
public:
    CProducer(const char* name,
              std::array<CMessagePort<int64_t>*, 10>& messagePorts, int consumerCount)
        : CTenant(name), messagePorts(messagePorts), consumerCount(consumerCount) {}

    void Run() override {
        int64_t now = k_uptime_get();
        for (int i = 0; i < consumerCount; i++) {
            if (messagePorts[i] != nullptr) {
                messagePorts[i]->Send(now);
            }
        }
    }

private:
    std::array<CMessagePort<int64_t>*, 10>& messagePorts;
    int consumerCount;
};

class CConsumer : public CTenant {
public:
    CConsumer(const char* name, CMessagePort<int64_t>& messagePort, int64_t* deltas, size_t deltaSize, int index)
        : CTenant(name), messagePort(messagePort), deltas(deltas), deltaSize(deltaSize), pollIndex(index) {}

    void Run() override {
        // All messages received so raise a bit in the signal to mark completion
        if (deltaIndex >= deltaSize) {
            int sigResult = 0;
            k_poll_signal_check(&consumersFinishedSignal, nullptr, &sigResult);
            sigResult |= (1 << pollIndex);
            k_poll_signal_raise(&consumersFinishedSignal, sigResult);
            return;
        }

        int64_t msgValue;
        if (messagePort.Receive(msgValue)) {
            deltas[deltaIndex++] = msgValue;
        }
    }

private:
    CMessagePort<int64_t>& messagePort;
    int64_t* deltas;
    size_t deltaIndex = 0;
    size_t deltaSize;
    int pollIndex;
};

typedef void (*RtosSetupFn)(CProducer producer, CConsumer consumers[], int consumerCount);

static void reportResults(const char* name, const int64_t deltas[], const size_t deltaSize) {
    printk("%s:", name);
    uint64_t sum = 0;
    for (size_t i = 0; i < deltaSize; i++) {
        printk("\tDelta %u: %lld", name, i, deltas[i]);
        sum += deltas[i];
    }
    printk("\tAverage: %lld\n", name, sum / deltaSize);
}

static void benchmarkMsgq(RtosSetupFn* rtosSetupFn, int consumerCount = 1, int deltaSize = 100) {
    static_assert(consumerCount > 0, "Must have at least one consumer");
    static_assert(consumerCount <= 10, "Cannot have more than 10 consumers");

    static constexpr size_t maxQueues = 10;
    static constexpr size_t maxDeltas = 100;
    static int64_t allDeltas[maxQueues][maxDeltas * sizeof(int64_t)] = {0};

    CMsgqMessagePort<int64_t> msgqPorts[10] = {
        CMsgqMessagePort<int64_t>(*queues[0]),
        CMsgqMessagePort<int64_t>(*queues[1]),
        CMsgqMessagePort<int64_t>(*queues[2]),
        CMsgqMessagePort<int64_t>(*queues[3]),
        CMsgqMessagePort<int64_t>(*queues[4]),
        CMsgqMessagePort<int64_t>(*queues[5]),
        CMsgqMessagePort<int64_t>(*queues[6]),
        CMsgqMessagePort<int64_t>(*queues[7]),
        CMsgqMessagePort<int64_t>(*queues[8]),
        CMsgqMessagePort<int64_t>(*queues[9]),
    };

    std::array<CMessagePort<int64_t>*, 10> producerPorts = {
        &msgqPorts[0],
        &msgqPorts[1],
        &msgqPorts[2],
        &msgqPorts[3],
        &msgqPorts[4],
        &msgqPorts[5],
        &msgqPorts[6],
        &msgqPorts[7],
        &msgqPorts[8],
        &msgqPorts[9],
    };

    CConsumer consumers[10] = {
        CConsumer("Consumer0", msgqPorts[0], allDeltas[0], deltaSize, 0),
        CConsumer("Consumer1", msgqPorts[1], allDeltas[1], deltaSize, 1),
        CConsumer("Consumer2", msgqPorts[2], allDeltas[2], deltaSize, 2),
        CConsumer("Consumer3", msgqPorts[3], allDeltas[3], deltaSize, 3),
        CConsumer("Consumer4", msgqPorts[4], allDeltas[4], deltaSize, 4),
        CConsumer("Consumer5", msgqPorts[5], allDeltas[5], deltaSize, 5),
        CConsumer("Consumer6", msgqPorts[6], allDeltas[6], deltaSize, 6),
        CConsumer("Consumer7", msgqPorts[7], allDeltas[7], deltaSize, 7),
        CConsumer("Consumer8", msgqPorts[8], allDeltas[8], deltaSize, 8),
        CConsumer("Consumer9", msgqPorts[9], allDeltas[9], deltaSize, 9),
    };

    CProducer producer("Producer", producerPorts, consumerCount);

    k_poll_signal_reset(&consumersFinishedSignal);

    rtosSetupFn(producer, consumers, consumerCount);
    NRtos::ClearTasks();
    NRtos::StartRtos();

    k_poll_event event = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                 K_POLL_MODE_NOTIFY_ONLY,
                                 &consumersFinishedSignal);

    while (true) {
        k_poll(&event, consumerCount, K_FOREVER);
        int sigResult = 0;
        k_poll_signal_check(&consumersFinishedSignal, nullptr, &sigResult);
        int result = 0;
        for (int i = 0; i < consumerCount; i++) {
            result |= (1 << i);
        }
        if (sigResult == result) {
            break;
        }
    }
    NRtos::StopRtos();
    for (int i = 0; i < consumerCount; i++) {
        char name[32] = {0};
        snprintf(name, sizeof(name), "Consumer%d", i);
        reportResults(name, allDeltas[i], deltaSize);
        memset(allDeltas[i], 0, deltaSize * sizeof(int64_t));
        k_msgq_purge(queues[i]);
    }
}

int main() {
    k_poll_signal_init(&consumersFinishedSignal);

    LOG_INF("Starting k_msgq benchmark...");

    benchmarkMsgq(3, 100);
    return 0;
}
