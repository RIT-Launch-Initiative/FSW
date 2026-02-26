#include "c_cmsismatrix.hpp"
#include "c_manualmatrix.hpp"

#include <cstdint>

namespace NBenchMark {
volatile float x[16] = {1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4};
volatile float y[4] = {1, 2, 3, 4};

int benchmark() {

    ManualMatrix<4, 4> m1 = ManualMatrix<4, 4>::Zeros();
    CMSISMatrix<4, 4> c1 = CMSISMatrix<4, 4>::Zeros();

    for (std::size_t i = 0; i < 4; i++) {
        for (std::size_t j = 0; j < 4; j++) {
            m1.Set(i, j, x[(i * 4) + j]);
            c1.Set(i, j, x[(i * 4) + j]);
        }
    }

    ManualMatrix<4, 1> m2 = ManualMatrix<4, 1>::Zeros();
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
    return 0;
}
} // namespace NBenchMark