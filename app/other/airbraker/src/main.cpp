#include "cmsismatrix.hpp"
#include "matrix.hpp"
#include "n_model.h"
#include "n_storage.h"
#include "servo.h"

#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

SYS_INIT(servo_init, APPLICATION, 1);
SYS_INIT(storage_init, APPLICATION, 2);

volatile float x[16] = {1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4};
volatile float y[4] = {1, 2, 3, 4};

volatile float alt = 1.0;
volatile float acc = 0;

int main() {
    using namespace NModel;

    k_msleep(1000);

    printk("starting....\n");

    k_msleep(1000);

    

    Matrix<4, 4> m1 = Matrix<4, 4>::Zeros();
    CMSISMatrix<4, 4> c1 = CMSISMatrix<4, 4>::Zeros();
    for (std::size_t i = 0; i < 4; i++) {
        for (std::size_t j = 0; j < 4; j++) {
            m1.Set(i, j, x[(i * 4) + j]);
            c1.Set(i, j, x[(i * 4) + j]);
        }
    }

    Matrix<4, 1> m2 = Matrix<4, 1>::Zeros();
    CMSISMatrix<4, 1> c2 = CMSISMatrix<4, 1>::Zeros();
    for (std::size_t i = 0; i < 4; i++) {
        m2.Set(i, 0, x[i]);
        c2.Set(i, 0, x[i]);
    }

    auto res = irq_lock();
    uint32_t startM = k_cycle_get_32();
    auto m3 = m1 * m2;
    uint32_t deltaM = k_cycle_get_32() - startM;

    uint32_t startC = k_cycle_get_32();
    auto c3 = c1 * c2;
    uint32_t deltaC = k_cycle_get_32() - startC;

    uint32_t addStartM = k_cycle_get_32();
    auto m4 = m1 + m1;
    uint32_t addDeltaM = k_cycle_get_32() - addStartM;

    uint32_t addStartC = k_cycle_get_32();
    auto c4 = c1 + c1;
    uint32_t addDeltaC = k_cycle_get_32() - addStartC;
    irq_unlock(res);

    printk("\n\n\n\n\nResults====================================================\n");

    k_msleep(10);
    printk("Done\n");

    printk("M1:\n");
    for (std::size_t i = 0; i < 4; i++) {
        for (std::size_t j = 0; j < 4; j++) {
            printk("%d,%d: %f, %f\n", i, j, (double) m1.Get(i, j), (double) c1.Get(i, j));
        }
    }

    k_msleep(10);

    printk("Add result:\n");
    for (std::size_t i = 0; i < 4; i++) {
        for (std::size_t j = 0; j < 4; j++) {
            printk("%d,%d: %f, %f\n", i, j, (double) m4.Get(i, j), (double) c4.Get(i, j));
        }
    }
    k_msleep(10);
    printk("Mult result:\n");
    for (std::size_t i = 0; i < 4; i++) {
        printk("%d: %f, %f\n", i, (double) m3.Get(i, 0), (double) c3.Get(i, 0));
    }

    k_msleep(10);
    printk("Addition Matrix: (4x4 + 4x4) %d us, %d cycles\n", k_cyc_to_us_near32(addDeltaC), addDeltaC);
    printk("Addition CMSIS:  (4x4 + 4x4) %d us, %d cycles\n", k_cyc_to_us_near32(addDeltaM), addDeltaM);

    printk("Multiply Matrix: (4x4 * 4x1) %d us, %d cycles\n", k_cyc_to_us_near32(deltaC), deltaC);
    printk("Multiply CMSIS:  (4x4 * 4x1) %d us, %d cycles\n", k_cyc_to_us_near32(deltaM), deltaM);

    k_msleep(10);
    printk("Time to break air\n");
    // StateT state = StateT::Column({0, 0, 0, 9.8});
    // printk("state:  %f, %f, %f, %f\n", (double) state.vals[0][0], (double) state.vals[1][0], (double) state.vals[2][0],
    //        (double) state.vals[3][0]);
    // printk("sensor: %f, %f\n", (double) alt, (double) acc);

    // StateT newState = kalman_update_and_predict(state, alt, acc);
    // printk("state': %f, %f, %f, %f\n", (double) newState.vals[0][0], (double) newState.vals[1][0], (double) newState.vals[2][0], (double) newState.vals[3][0]);
}
