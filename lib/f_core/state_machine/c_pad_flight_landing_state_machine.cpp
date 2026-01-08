#include "f_core/state_machine/c_pad_flight_landing_state_machine.h"

namespace NStateMachineGlobals {
    volatile bool boostDetected = false;
    volatile bool landingDetected = false;
    k_mutex boostMutex;
    k_mutex landingMutex;
}

CPadFlightLandedStateMachine::CPadFlightLandedStateMachine()
    : state(State::PAD), boostDetected(false), landingDetected(false) {
    k_mutex_init(&NStateMachineGlobals::boostMutex);
    k_mutex_init(&NStateMachineGlobals::landingMutex);
}

void CPadFlightLandedStateMachine::Clock() {
    switch (state) {
    case State::PAD:
        PadRun();
        if (boostDetected) {
            state = State::FLIGHT;
            stateEntryFunctionHandled = false;
            PadExit();
        }
        break;
    case State::FLIGHT:
        if (!stateEntryFunctionHandled) {
            FlightEntry();
            stateEntryFunctionHandled = true;
        }

        FlightRun();
        if (landingDetected) {
            state = State::LANDED;
            stateEntryFunctionHandled = false;
            FlightExit();
        }
        break;
    case State::LANDED:
        if (!stateEntryFunctionHandled) {
            LandedEntry();
            stateEntryFunctionHandled = true;
        }

        LandedRun();
        state = State::GROUND;
        break;
    case State::GROUND:
        GroundRun();
        break;
    }
}

