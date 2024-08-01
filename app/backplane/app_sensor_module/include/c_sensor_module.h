#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include <f_core/c_project_configuration.h>

class CSensorModule : public CProjectConfiguration {
public:
    CProjectConfiguration* GetInstance() override;

private:
    CSensorModule();

    void addTenants() override;

    void addTasks() override;

    void setupMessagePorts() override {};

    void setupCallbacks() override {};
};



#endif //C_SENSOR_MODULE_H
