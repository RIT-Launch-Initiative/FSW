#ifndef C_PYRO_CONTROL_OBSERVER_H
#define C_PYRO_CONTROL_OBSERVER_H

#include <array>
#include <f_core/device/c_gpio.h>
#include <f_core/os/c_file.h>
#include <f_core/os/flight_log.hpp>
#include <f_core/utils/c_observer.h>


class CPyroControlObserver : public CObserver {
public:
    CPyroControlObserver() = default;

    ~CPyroControlObserver() override = default;

    /**
     * See parent docs
     * @param ctx Alert number
     */
    void Notify(void* ctx) override;

private:
    struct PyroPair {
        CGpio sense;
        CGpio ctrl;
    };


    std::array<CGpio, 4> de9Gpios{
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio0), gpios)),
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio1), gpios)),
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio2), gpios)),
        CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(gpio3), gpios))
    };

    CFlightLog flightLog{"/lfs/log.txt"};

    std::array<PyroPair, 4> pyroPairs{
        PyroPair{
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_sns_0), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_0), gpios))
        },
        PyroPair{
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_sns_1), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_1), gpios))
        },
        PyroPair{
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_sns_2), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_2), gpios))
        },
        PyroPair{
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_sns_3), gpios)),
            CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(pyro_ctrl_3), gpios))
        },
    };
};


#endif //C_PYRO_CONTROL_OBSERVER_H
