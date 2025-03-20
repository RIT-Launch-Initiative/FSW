#ifndef C_STATE_MACHINE_UPDATER_H
#define C_STATE_MACHINE_UPDATER_H

#include <f_core/utils/c_observer.h>

class CStateMachineUpdater : public CObserver {
public:
    /**
     * Constructor
     */
    CStateMachineUpdater() = default;

    /**
     * Destructor
     */
    ~CStateMachineUpdater() override = default;

    /**
     * See parent docs
     */
    void Notify(void *ctx) override;
};

#endif //C_STATE_MACHINE_UPDATER_H
