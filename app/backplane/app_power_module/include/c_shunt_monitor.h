#ifndef C_SHUNT_MONITOR_H
#define C_SHUNT_MONITOR_H

// F-Core Includes
#include <f_core/os/c_task.h>

class CSensorDevice;

class CShuntMonitor : public CTask {
public:
    CShuntMonitor(const char *name, uint8_t priority, uint32_t stackSize) : CTask(name, priority, stackSize) {}

    void Run() override;

private:
    CSensorDevice &ina3v3;
    CSensorDevice &ina5v0;
    CSensorDevice &inaBatt;
};



#endif //C_SHUNT_MONITOR_H
