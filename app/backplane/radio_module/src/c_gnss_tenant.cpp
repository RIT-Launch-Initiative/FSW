#include "c_gnss_tenant.h"
#include "c_radio_module.h"

#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <f_core/device/sensor/c_magnetometer.h>
#include <f_core/device/sensor/c_temperature_sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CGnssTenant);

extern k_msgq broadcastQueue;

void CGnssTenant::Startup() {

}

void CGnssTenant::PostStartup() {

}

void CGnssTenant::Run() {

}
