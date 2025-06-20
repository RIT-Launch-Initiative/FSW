#include "c_sensing_tenant.h"
#include "f_core/messaging/c_msgq_message_port.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(sensor_mod, CONFIG_LOG_DEFAULT_LEVEL);

extern void flashLogEnable();
extern void flashLogDisable();

static int cmdLogOn(const shell* sh, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    flashLogEnable();
    shell_print(sh, "Flash logging enabled");

    return 0;
}

static int cmdLogOff(const shell* sh, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    flashLogDisable();
    shell_print(sh, "Flash logging disabled");

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_log,
                               SHELL_CMD_ARG(on, NULL, "Enable sensor data logging to flash", cmdLogOn, 1, 0),
                               SHELL_CMD_ARG(off, NULL, "Disable sensor data logging to flash", cmdLogOff, 1, 0),
                               SHELL_SUBCMD_SET_END
    );

SHELL_CMD_REGISTER(log, &sub_log, "Sensor data logging commands", NULL);

K_MSGQ_DEFINE(broadcastQueue, sizeof(NTypes::SensorData), 1, 4);
static auto broadcastMsgQueue = CMsgqMessagePort<NTypes::SensorData>(broadcastQueue);

K_MSGQ_DEFINE(downlinkQueue, sizeof(NTypes::LoRaBroadcastSensorData), 1, 4);
static auto downlinkMsgQueue = CMsgqMessagePort<NTypes::LoRaBroadcastSensorData>(downlinkQueue);

K_MSGQ_DEFINE(dataLogQueue, sizeof(NTypes::TimestampedSensorData), 1, 4);
static auto dataLogMsgQueue = CMsgqMessagePort<NTypes::TimestampedSensorData>(dataLogQueue);

K_MSGQ_DEFINE(alertQueue, sizeof(NAlerts::AlertPacket), 1, 4);
static auto alertMsgQueue = CMsgqMessagePort<NAlerts::AlertPacket>(alertQueue);

extern void flashListenerInit();
extern void ethListenerInit();
extern void sensorsInit();
extern void readSensors();

int main() {
    flashListenerInit();
    ethListenerInit();
    sensorsInit();
    while (true) {
        readSensors();
    }

    return 0;
}
