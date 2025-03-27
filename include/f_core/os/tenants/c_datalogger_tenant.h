#ifndef C_DATALOGGER_TENANT_H
#define C_DATALOGGER_TENANT_H

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>
#include <f_core/os/c_datalogger.h>
#include <zephyr/logging/log.h>

template <typename T>
class CDataLoggerTenant : public CTenant {
public:
    CDataLoggerTenant(const char *name, const char *filename, LogMode mode, std::size_t num_packets, CMessagePort<T> &messagePort)
        : CTenant(name), messagePort(messagePort), dataLogger(filename, mode, num_packets), filename(filename) {}

    ~CDataLoggerTenant() override {
        Cleanup();
    }

    void Run() override {
        if (T message{}; messagePort.Receive(message, K_FOREVER) == 0) {
            dataLogger.write(message);
        }
    }

    void Cleanup() override {
        dataLogger.close();
    }

private:
    CMessagePort<T> &messagePort;
    CDataLogger<T> dataLogger;
    const char *filename;
};

#endif //C_DATALOGGER_TENANT_H
