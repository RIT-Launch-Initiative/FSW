#ifndef PADFLIGHTSTATEMACHINE_H
#define PADFLIGHTSTATEMACHINE_H

// We shouldn't define globals, but this is a quick and dirty way
// to avoid a bunch of inter-thread communication. Should
namespace StateMachineGlobals {
    bool boostDetected = false;
    bool landingDetected = false;
    bool groundModule = false;
}

class PadFlightLandedStateMachine {
public:
    enum class State {
        PAD = 0,
        FLIGHT,
        LANDED,
        GROUND
    };

    PadFlightLandedStateMachine();

protected:
    State state;

    void Clock() {
        if (StateMachineGlobals::groundModule) {
            state = State::GROUND;
        }

        switch (state) {
            case State::PAD:
                PadRun();

                if (boostDetected) {
                    state = State::FLIGHT;
                }

                if (StateMachineGlobals::groundModule) {
                    state = State::GROUND;
                }

                break;
            case State::FLIGHT:
                FlightRun();

                if (landingDetected) {
                    state = State::LANDED;
                }
                break;
            case State::LANDED:
                LandedRun();
                break;

            case State::GROUND:
                GroundRun();
                break;

            default:
                break;
        }
    }

    void SetBoostDetected(bool detected) {
        boostDetected = detected;
    }

    void SetLandingDetected(bool detected) {
        landingDetected = detected;
    }

    virtual void PadRun() = 0;

    virtual void FlightRun() = 0;

    virtual void LandedRun() = 0;

    virtual void GroundRun() = 0;

private:
    bool boostDetected;
    bool landingDetected;
};

#endif //PADFLIGHTSTATEMACHINE_H
