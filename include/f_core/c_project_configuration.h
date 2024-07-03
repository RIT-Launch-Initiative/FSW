#ifndef C_PROJECT_CONFIGURATION_H
#define C_PROJECT_CONFIGURATION_H

class CProjectConfiguration {
public:
    CProjectConfiguration();

    virtual void SetupTasks() = 0;

protected:
    ~CProjectConfiguration();
};

#endif //C_PROJECT_CONFIGURATION_H
