#pragma once

class CProjectConfiguration {
public:
 virtual ~CProjectConfiguration() = default;

protected:
    /**
     * Constructor
     */
    CProjectConfiguration() = default;

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




