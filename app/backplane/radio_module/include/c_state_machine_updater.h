#pragma once

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


