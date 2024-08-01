//
// Created by aaron on 7/31/24.
//

#include "c_sensor_module.h"

CSensorModule::CSensorModule() : CProjectConfiguration() {
    addTenants();
    addTasks();
}


CProjectConfiguration* CSensorModule::GetInstance() {

    return &instance;
}

