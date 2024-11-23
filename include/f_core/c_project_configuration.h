#ifndef C_PROJECT_CONFIGURATION_H
#define C_PROJECT_CONFIGURATION_H

class CProjectConfiguration {
protected:
    /**
     * Constructor
     */
    CProjectConfiguration() = default;

    ~CProjectConfiguration() = default;
private:
    /**
     * Add tenants to a task
     */
    virtual void AddTenantsToTasks() = 0;

    /*
     * Add tasks to RTOS
     */
    virtual void AddTasksToRtos() = 0;

    /**
     * Setup callbacks
     */
    virtual void SetupCallbacks() = 0;
};



#endif //C_PROJECT_CONFIGURATION_H
