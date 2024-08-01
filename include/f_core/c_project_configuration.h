#ifndef C_PROJECT_CONFIGURATION_H
#define C_PROJECT_CONFIGURATION_H

class CProjectConfiguration {
protected:
    /**
     * Constructor
     */
    CProjectConfiguration() = default;
private:
    /**
     * Add tenants to a task
     */
    virtual void addTenantsToTasks() = 0;

    /*
     * Add tasks to RTOS
     */
    virtual void addTasksToRtos() = 0;

    /**
     * Setup callbacks
     */
    virtual void setupCallbacks() = 0;
};



#endif //C_PROJECT_CONFIGURATION_H
