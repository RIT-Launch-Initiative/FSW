#ifndef C_PROJECT_CONFIGURATION_H
#define C_PROJECT_CONFIGURATION_H



class CProjectConfiguration {
public:
    CProjectConfiguration *GetInstance() {
        return instance;
    };

protected:
    explicit consteval CProjectConfiguration(CProjectConfiguration *instance) : instance(instance) {};

    CProjectConfiguration *instance;
private:

    virtual void addTenants();

    virtual void addTasks();

    virtual void setupMessagePorts();

    virtual void setupCallbacks();
};



#endif //C_PROJECT_CONFIGURATION_H
