#ifndef C_ADC_MONITOR_H
#define C_ADC_MONITOR_H

#include <f_core/os/c_task.h>

class CAdcMonitor : public CTask {
public:
    CAdcMonitor(const char *name, uint8_t priority, uint32_t stackSize) : CTask(name, priority, stackSize) {}

    void Run() override;
private:
    // TODO: ADC class
};



#endif //C_ADC_MONITOR_H
