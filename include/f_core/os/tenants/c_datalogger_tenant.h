#ifndef C_DATALOGGER_TENANT_H
#define C_DATALOGGER_TENANT_H

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>
#include <f_core/os/c_datalogger.h>

template <typename T>
class CDataLoggerTenant : public CTenant {
public:
    CDataLoggerTenant(const char *name, const char *filename, LogMode mode, std::size_t num_packets, CMessagePort<T> &messagePort)
        : CTenant(name), messagePort(messagePort), dataLogger(filename, mode, num_packets) {}

    ~CDataLoggerTenant() override {
        dataLogger.close();
    }

    void Run() override {
        while (true) {
            if (T message{}; messagePort.Receive(message, K_FOREVER) == 0) {
                dataLogger.write(message);
            }
        }
    }

private:
    CMessagePort<T> &messagePort;
    CDataLogger<T> dataLogger;
};

#endif //C_DATALOGGER_TENANT_H
