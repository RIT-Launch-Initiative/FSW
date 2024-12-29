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
#include <f_core/messaging/c_zbus_message_port.h>

LOG_MODULE_REGISTER(main);

K_MUTEX_DEFINE(consumersFinishedMutex);

K_MSGQ_DEFINE(queueOne, sizeof(uint64_t), 10, 4);
K_MSGQ_DEFINE(queueTwo, sizeof(uint64_t), 10, 4);
K_MSGQ_DEFINE(queueThree, sizeof(uint64_t), 10, 4);
K_MSGQ_DEFINE(queueFour, sizeof(uint64_t), 10, 4);
K_MSGQ_DEFINE(queueFive, sizeof(uint64_t), 10, 4);
K_MSGQ_DEFINE(queueSix, sizeof(uint64_t), 10, 4);
K_MSGQ_DEFINE(queueSeven, sizeof(uint64_t), 10, 4);
K_MSGQ_DEFINE(queueEight, sizeof(uint64_t), 10, 4);
K_MSGQ_DEFINE(queueNine, sizeof(uint64_t), 10, 4);
K_MSGQ_DEFINE(queueTen, sizeof(uint64_t), 10, 4);

ZBUS_CHAN_DEFINE(cyclesChannel,        /* Name */
                 int64_t,              /* Message type */
                 NULL,                 /* Validator */
                 NULL,                 /* User data */
                 ZBUS_OBSERVERS_EMPTY, /* observers */
                 0                     /* Initial value is 0 */
    );

static constexpr std::array<k_msgq*, 10> queues = {
    &queueOne, &queueTwo, &queueThree, &queueFour, &queueFive,
    &queueSix, &queueSeven, &queueEight, &queueNine, &queueTen
};

static volatile int consumersFinished = 0;

static void consumerFinished() {
    k_mutex_lock(&consumersFinishedMutex, K_FOREVER);
    consumersFinished++;
    k_mutex_unlock(&consumersFinishedMutex);
}

static void waitForConsumersAndClear(int consumerCount) {
    while (true) {
        k_sleep(K_MSEC(100));
        k_mutex_lock(&consumersFinishedMutex, K_FOREVER);
        if (consumersFinished >= consumerCount) {
            k_mutex_unlock(&consumersFinishedMutex);
            break;
        }
        k_mutex_unlock(&consumersFinishedMutex);
    }
    consumersFinished = 0;
}

class CProducer : public CTenant {
public:
    CProducer(const char* name,
              std::array<CMessagePort<uint64_t>*, 10>& messagePorts, int consumerCount)
        : CTenant(name), messagePorts(messagePorts), consumerCount(consumerCount) {}

    void Run() override {
        uint64_t now = k_cycle_get_64();
        for (int i = 0; i < consumerCount; i++) {
            if (messagePorts[i] != nullptr) {
                int ret = messagePorts[i]->Send(now);
                if (ret != 0) {
                    LOG_ERR("Failed to send message");
                }
            } else {
                LOG_ERR("Message port is null");
                k_oops();
            }
        }
    }

private:
    std::array<CMessagePort<uint64_t>*, 10>& messagePorts;
    int consumerCount;
};

class CConsumer : public CTenant {
public:
    CConsumer(const char* name, CMessagePort<uint64_t>& messagePort, uint64_t* deltas, size_t deltaSize, int index)
        : CTenant(name), messagePort(messagePort), deltas(deltas), deltaSize(deltaSize), pollIndex(index) {}

    void Run() override {
        if (deltaIndex >= deltaSize) {
            if (!finished) {
                consumerFinished();
                finished = true;
            }
            return;
        }

        uint64_t start;
        if (int ret = messagePort.Receive(start); ret == 0) {
            deltas[deltaIndex++] = k_cycle_get_64() - start;
        } else {
            LOG_ERR("Failed to receive message %d", ret);
        }
    }

private:
    CMessagePort<uint64_t>& messagePort;
    uint64_t* deltas;
    size_t deltaIndex = 0;
    size_t deltaSize;
    int pollIndex;
    bool finished = false;
};

using RtosSetupFn = void (*)(CProducer& producer, CConsumer consumers[], int consumerCount);

static uint64_t reportResults(const char* name, const uint64_t deltas[], const size_t deltaSize) {
    printk("%s:\n", name);
    uint64_t sum = 0;
    for (size_t i = 0; i < deltaSize; i++) {
        uint64_t microseconds = k_cyc_to_us_floor64(deltas[i]);
        printk("\tDelta %zu: %lld us\n", i, microseconds);
        sum += microseconds;
    }

    uint64_t average = sum / deltaSize;
    printk("\tAverage: %lld us\n", average);
    return average;
}


void benchmarkMsgq(RtosSetupFn rtosSetupFn, int consumerCount, int deltaSize) {
    static constexpr size_t maxQueues = 10;
    static constexpr size_t maxDeltas = 100;
    static uint64_t allDeltas[maxQueues][maxDeltas * sizeof(uint64_t)] = {0};

    CMsgqMessagePort<uint64_t> msgqPorts[10] = {
        CMsgqMessagePort<uint64_t>(*queues[0]),
        CMsgqMessagePort<uint64_t>(*queues[1]),
        CMsgqMessagePort<uint64_t>(*queues[2]),
        CMsgqMessagePort<uint64_t>(*queues[3]),
        CMsgqMessagePort<uint64_t>(*queues[4]),
        CMsgqMessagePort<uint64_t>(*queues[5]),
        CMsgqMessagePort<uint64_t>(*queues[6]),
        CMsgqMessagePort<uint64_t>(*queues[7]),
        CMsgqMessagePort<uint64_t>(*queues[8]),
        CMsgqMessagePort<uint64_t>(*queues[9]),
    };

    std::array<CMessagePort<uint64_t>*, 10> producerPorts = {
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

    NRtos::ClearTasks();

    rtosSetupFn(producer, consumers, consumerCount);

    NRtos::StartRtos();
    waitForConsumersAndClear(consumerCount);
    NRtos::StopRtos();

    uint64_t overallBenchmarkSum = 0;
    for (int i = 0; i < consumerCount; i++) {
        char name[32] = {0};
        snprintf(name, sizeof(name), "Consumer%d", i);
        overallBenchmarkSum = reportResults(name, allDeltas[i], deltaSize);
        memset(allDeltas[i], 0, deltaSize * sizeof(uint64_t));
        k_msgq_purge(queues[i]);
    }
    printk("\tOverall Average: %lld us\n", overallBenchmarkSum);
}

void benchmarkZbus(RtosSetupFn rtosSetupFn, int consumerCount, int deltaSize) {
    static constexpr size_t maxQueues = 10;
    static constexpr size_t maxDeltas = 100;
    static uint64_t allDeltas[maxQueues][maxDeltas * sizeof(uint64_t)] = {0};

    CZbusMessagePort<uint64_t> messagePort[1] = {
        CZbusMessagePort<uint64_t>(cyclesChannel)
    };

    std::array<CMessagePort<uint64_t>*, 10> producerPorts = {
        &messagePort[0],
        &messagePort[0],
        &messagePort[0],
        &messagePort[0],
        &messagePort[0],
        &messagePort[0],
        &messagePort[0],
        &messagePort[0],
        &messagePort[0],
        &messagePort[0],
    };

    CConsumer consumers[10] = {
        CConsumer("Consumer0", messagePort[0], allDeltas[0], deltaSize, 0),
        CConsumer("Consumer1", messagePort[0], allDeltas[1], deltaSize, 1),
        CConsumer("Consumer2", messagePort[0], allDeltas[2], deltaSize, 2),
        CConsumer("Consumer3", messagePort[0], allDeltas[3], deltaSize, 3),
        CConsumer("Consumer4", messagePort[0], allDeltas[4], deltaSize, 4),
        CConsumer("Consumer5", messagePort[0], allDeltas[5], deltaSize, 5),
        CConsumer("Consumer6", messagePort[0], allDeltas[6], deltaSize, 6),
        CConsumer("Consumer7", messagePort[0], allDeltas[7], deltaSize, 7),
        CConsumer("Consumer8", messagePort[0], allDeltas[8], deltaSize, 8),
        CConsumer("Consumer9", messagePort[0], allDeltas[9], deltaSize, 9),
    };

    CProducer producer("Producer", producerPorts, consumerCount);

    NRtos::ClearTasks();

    rtosSetupFn(producer, consumers, consumerCount);

    NRtos::StartRtos();
    waitForConsumersAndClear(consumerCount);
    NRtos::StopRtos();

    uint64_t overallBenchmarkSum = 0;
    for (int i = 0; i < consumerCount; i++) {
        char name[32] = {0};
        snprintf(name, sizeof(name), "Consumer%d", i);
        overallBenchmarkSum = reportResults(name, allDeltas[i], deltaSize);
        memset(allDeltas[i], 0, deltaSize * sizeof(uint64_t));
        k_msgq_purge(queues[i]);
    }
    printk("\tOverall Average: %lld us\n", overallBenchmarkSum);
}

void setupOneProducerOneConsumer(CProducer& producer, CConsumer consumers[], int) {
    LOG_INF("1 CONSUMER / 1 THREAD");

    static CTask producerTask("Producer Task", 15, 512);
    static CTask consumerTask("Consumer Task", 15, 512);

    producerTask.AddTenant(producer);
    consumerTask.AddTenant(consumers[0]);

    NRtos::AddTask(producerTask);
    NRtos::AddTask(consumerTask);
}

void setupOneProducerThreeConsumersTwoThread(CProducer& producer, CConsumer consumers[], int) {
    LOG_INF("3 CONSUMER / 1 THREAD");

    static CTask producerTask("Producer Task", 15, 512);
    static CTask consumerTask("Consumer Task 1", 15, 512);

    producerTask.AddTenant(producer);
    consumerTask.AddTenant(consumers[0]);
    consumerTask.AddTenant(consumers[1]);
    consumerTask.AddTenant(consumers[2]);

    NRtos::AddTask(producerTask);
    NRtos::AddTask(consumerTask);
}

void setupOneProducerThreeConsumersFourThread(CProducer& producer, CConsumer consumers[], int) {
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
    // benchmarkMsgq(setupOneProducerOneConsumer, 1, 10);
    // benchmarkMsgq(setupOneProducerThreeConsumersTwoThread, 3, 10);
    // benchmarkMsgq(setupOneProducerThreeConsumersFourThread, 3, 10);

    benchmarkZbus(setupOneProducerOneConsumer, 1, 10);
    benchmarkZbus(setupOneProducerThreeConsumersTwoThread, 3, 10);
    benchmarkZbus(setupOneProducerThreeConsumersFourThread, 3, 10);

    LOG_INF("Benchmarks complete!");
    return 0;
}
