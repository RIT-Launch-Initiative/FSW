#ifndef PADFLIGHTSTATEMACHINE_H
#define PADFLIGHTSTATEMACHINE_H

// We shouldn't define globals, but this is a quick and dirty way
// to avoid a bunch of inter-thread communication.
namespace NStateMachineGlobals {
    static volatile bool boostDetected = false;
    static volatile bool landingDetected = false;
}

class CPadFlightLandedStateMachine {
public:
    enum class State {
        PAD = 0,
        FLIGHT,
        LANDED,
        GROUND
    };

    CPadFlightLandedStateMachine() : state(State::PAD), boostDetected(false), landingDetected(false), isGroundModule(false) {};

protected:
    State state;

    void Clock() {
        if (isGroundModule) {
            state = State::GROUND;
        }

        switch (state) {
            case State::PAD:
                PadRun();

                if (boostDetected) {
                    state = State::FLIGHT;
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

    void SetIsGroundModule(const bool is) {
        isGroundModule = is;
    }

    virtual void PadRun() = 0;

    virtual void FlightRun() = 0;

    virtual void LandedRun() = 0;

    virtual void GroundRun() = 0;

private:
    bool boostDetected;
    bool landingDetected;
    bool isGroundModule;
};

#endif //PADFLIGHTSTATEMACHINE_H
