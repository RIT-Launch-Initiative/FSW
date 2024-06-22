#ifndef C_TENANT_H
#define C_TENANT_H

class CTenant {
public:
    CTenant(const char *name);

    virtual void Startup() = 0;

    virtual void PostStartup() = 0;

    virtual void Run() = 0;
private:
    const char *name;

};

#endif //C_TENANT_H
