#ifndef C_PROJECT_CONFIGURATION_H
#define C_PROJECT_CONFIGURATION_H

class CProjectConfiguration {
protected:
    CProjectConfiguration() = default;
private:
    virtual void addTenants() = 0;

    virtual void addTasks() = 0;

    virtual void setupMessagePorts() = 0;

    virtual void setupCallbacks() = 0;
};



#endif //C_PROJECT_CONFIGURATION_H
