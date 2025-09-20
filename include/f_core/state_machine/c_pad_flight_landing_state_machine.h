#ifndef PADFLIGHTSTATEMACHINE_H
#define PADFLIGHTSTATEMACHINE_H

#include <zephyr/kernel.h>

namespace NStateMachineGlobals {
    extern volatile bool boostDetected;
    extern volatile bool landingDetected;
    extern k_mutex boostMutex;
    extern k_mutex landingMutex;
}

class CPadFlightLandedStateMachine {
public:
    enum class State {
        PAD = 0,
        FLIGHT,
        LANDED,
        GROUND
    };

    CPadFlightLandedStateMachine();

protected:
    State state;

    void Clock();

    void SetBoostDetected(bool detected) {
        k_mutex_lock(&NStateMachineGlobals::boostMutex, K_SECONDS(1));
        boostDetected = detected;
        k_mutex_unlock(&NStateMachineGlobals::boostMutex);
    }

    void SetLandingDetected(bool detected) {
        k_mutex_lock(&NStateMachineGlobals::landingMutex, K_SECONDS(1));
        landingDetected = detected;
        k_mutex_unlock(&NStateMachineGlobals::landingMutex);
    }

    virtual void PadRun() = 0;

    virtual void FlightRun() = 0;

    virtual void LandedRun() = 0;

private:
    bool boostDetected;
    bool landingDetected;
};

#endif //PADFLIGHTSTATEMACHINE_H
