#pragma once
#ifdef CONFIG_CMSIS_DSP_MATRIX
#include "c_cmsismatrix.hpp"

template <std::size_t R, std::size_t C>
using Matrix = CMSISMatrix<R, C>;

#else

#include "c_manualmatrix.hpp"
template <std::size_t R, std::size_t C>
using Matrix = ManualMatrix<R, C, float>;

#endif
#include <zephyr/kernel.h>

void print3x3(const Matrix<3, 3> &mat);
template<std::size_t R, std::size_t C>
Matrix<R,C> matrixExp(const Matrix<R,C> &At, std::size_t iterations){
    float factorial = 1;
    Matrix<R,C> id = Matrix<R,C>::Identity();
    printk("id\n"); print3x3(id);

    Matrix<R,C> sum = Matrix<R,C>::Identity();
    
    Matrix<R,C> Atn = At;
    printk("Atn\n"); print3x3(Atn);
    
    for (std::size_t i = 1; i <= iterations; i++){
        factorial*=i;
        printk("factorial: %f", (double)factorial);
        float over_fac = (1.F / (float)factorial);
        printk("over fac = %f", (double)over_fac);
        Matrix<R, C> scaled = Atn * over_fac;
        printk("scaled\n"); print3x3(scaled);
        sum = sum + scaled;
        // sum = sum + (Atn * (1/(float)factorial));
        Matrix<R,C> Atn2  = Atn * At;
        Atn = Atn2;
        printk("ATn\n"); print3x3(Atn);
    }
    return sum;
}
