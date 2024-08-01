#include "c_sensor_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>

CSensorModule::CSensorModule() : CProjectConfiguration() {
    addTenants();
    addTasks();
}

void CSensorModule::addTenants() {
    networkTask.AddTenant(broadcastTenant);
    sensingTask.AddTenant(sensingTenant);
}

void CSensorModule::addTasks() {
    NRtos::AddTask(networkTask);
    NRtos::AddTask(sensingTask);
}



