#include "c_sensor_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>

CSensorModule::CSensorModule() : CProjectConfiguration() {
}

void CSensorModule::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(broadcastTenant);

    // Sensing
    sensingTask.AddTenant(sensingTenant);
}

void CSensorModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);

    // Sensing
    NRtos::AddTask(sensingTask);
}

void CSensorModule::SetupCallbacks() {
}



