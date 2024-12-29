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

LOG_MODULE_REGISTER(main);

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
            LOG_INF("PRODUCING!");
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
        LOG_INF("%d CONSUMING!", pollIndex);
        // All messages received so raise a bit in the signal to mark completion
        if (deltaIndex >= deltaSize) {
            int sigResult = 0;
            k_poll_signal_check(&consumersFinishedSignal, nullptr, &sigResult);
            sigResult |= (1 << pollIndex);
            k_poll_signal_raise(&consumersFinishedSignal, sigResult);
            LOG_INF("Done!");
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

using RtosSetupFn = void (*)(CProducer &producer, CConsumer consumers[], int consumerCount);

static void reportResults(const char* name, const int64_t deltas[], const size_t deltaSize) {
    LOG_PRINTK("%s:", name);
    uint64_t sum = 0;
    for (size_t i = 0; i < deltaSize; i++) {
        LOG_PRINTK("\tDelta %u: %lld", name, i, deltas[i]);
        sum += deltas[i];
    }
    LOG_PRINTK("\tAverage: %lld\n", name, sum / deltaSize);
}

void benchmarkMsgq(RtosSetupFn rtosSetupFn, int consumerCount, int deltaSize) {
    static constexpr size_t maxQueues = 10;
    static constexpr size_t maxDeltas = 100;
    static int64_t allDeltas[maxQueues][maxDeltas * sizeof(int64_t)] = {0};

    static CMsgqMessagePort<int64_t> msgqPorts[10] = {
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

    static std::array<CMessagePort<int64_t>*, 10> producerPorts = {
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

    static CConsumer consumers[10] = {
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

    static CProducer producer("Producer", producerPorts, consumerCount);

    k_poll_signal_reset(&consumersFinishedSignal);

    NRtos::ClearTasks();

    rtosSetupFn(producer, consumers, consumerCount);

    k_poll_event event{};
    k_poll_event_init(&event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &consumersFinishedSignal);

    NRtos::StartRtos();

    while (true) {
        LOG_INF("SLEEPING WAITING FOR CONSUMERS TO FINISH");
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

void setupOneProducerOneConsumer(CProducer &producer, CConsumer consumers[], int) {
    LOG_INF("1 CONSUMER / 1 THREAD");

    static CTask producerTask("Producer Task", 15, 512);
    static CTask consumerTask("Consumer Task", 15, 512);

    producerTask.AddTenant(producer);
    consumerTask.AddTenant(consumers[0]);

    NRtos::AddTask(producerTask);
    NRtos::AddTask(consumerTask);
}

void setupOneProducerThreeConsumersTwoThread(CProducer &producer, CConsumer consumers[], int) {
    LOG_INF("3 CONSUMER / 1 THREADS");

    static CTask producerTask("Producer Task", 15, 512);
    static CTask consumerTaskOne("Consumer Task 1", 15, 512);
    static CTask consumerTaskTwo("Consumer Task 2", 15, 512);

    producerTask.AddTenant(producer);
    consumerTaskOne.AddTenant(consumers[0]);
    consumerTaskTwo.AddTenant(consumers[1]);

    NRtos::AddTask(producerTask);
    NRtos::AddTask(consumerTaskOne);
    NRtos::AddTask(consumerTaskTwo);
}

void setupOneProducerThreeConsumersFourThread(CProducer &producer, CConsumer consumers[], int) {
    LOG_INF("3 CONSUMER / 3 THREADS");

    static CTask producerTask("Producer Task", 15, 1024);
    static CTask consumerTaskOne("Consumer Task 1", 15, 1024);
    static CTask consumerTaskTwo("Consumer Task 2", 15, 1024);
    static CTask consumerTaskThree("Consumer Task 3", 15, 1024);

    producerTask.AddTenant(producer);
    consumerTaskOne.AddTenant(consumers[0]);
    consumerTaskTwo.AddTenant(consumers[1]);
    consumerTaskThree.AddTenant(consumers[2]);

    NRtos::AddTask(producerTask);
    NRtos::AddTask(consumerTaskOne);
    NRtos::AddTask(consumerTaskTwo);
    NRtos::AddTask(consumerTaskThree);
}

int main() {
    k_poll_signal_init(&consumersFinishedSignal);

    benchmarkMsgq(setupOneProducerOneConsumer, 3, 100);
    benchmarkMsgq(setupOneProducerThreeConsumersTwoThread, 3, 100);
    benchmarkMsgq(setupOneProducerThreeConsumersFourThread, 3, 100);
    return 0;
}
