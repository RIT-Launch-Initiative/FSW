#ifndef C_PROJECT_CONFIGURATION_H
#define C_PROJECT_CONFIGURATION_H



class CProjectConfiguration {
public:
    virtual CProjectConfiguration *GetInstance();

private:
    CProjectConfiguration() = default;

    virtual void addTenants();

    virtual void addTasks();

    virtual void setupMessagePorts();

    virtual void setupCallbacks();
};



#endif //C_PROJECT_CONFIGURATION_H
