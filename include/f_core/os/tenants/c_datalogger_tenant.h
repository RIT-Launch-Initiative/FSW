#ifndef C_DATALOGGER_TENANT_H
#define C_DATALOGGER_TENANT_H

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>
#include <f_core/os/c_datalogger.h>
#include <zephyr/logging/log.h>


template <typename T>
class CDataLoggerTenant : public CTenant {
public:
    CDataLoggerTenant(const char* name, const char* filename, LogMode mode, std::size_t numPackets,
                      CMessagePort<T>& messagePort, k_timeout_t syncTimeout = K_FOREVER, int syncOnCount = 0)
        : CTenant(name), messagePort(messagePort), dataLogger(filename, mode, numPackets), filename(filename),
          syncTimeout(syncTimeout), syncOnCount(syncOnCount) {}

    ~CDataLoggerTenant() override {
        Cleanup();
    }

    void Startup() override {
        if (syncTimeout.ticks != K_FOREVER.ticks) {
            syncTimer.SetUserData(&dataLogger);
            syncTimer.StartTimer(syncTimeout, K_NO_WAIT);
        }
    }

    void Run() override {
        if (T message{}; messagePort.Receive(message, K_FOREVER) == 0) {
            dataLogger.Write(message);
            syncCounter++;
            if ((syncTimer.IsRunning() && syncTimer.IsExpired()) ||
                (syncOnCount > 0 && syncCounter >= syncOnCount)) {
                dataLogger.Sync();
                syncCounter = 0;
                if (syncTimer.IsRunning()) {
                    syncTimer.StartTimer(syncTimeout);
                }
            }
        }
    }

    void Cleanup() override {
        dataLogger.Close();
    }

private:
    CMessagePort<T>& messagePort;
    CDataLogger<T> dataLogger;
    const char* filename;

    // FS Sync after every X time
    CSoftTimer syncTimer{nullptr, nullptr};
    k_timeout_t syncTimeout = K_FOREVER;

    // FS Sync on every N messages
    int syncCounter = 0;
    int syncOnCount;
};

#endif //C_DATALOGGER_TENANT_H
