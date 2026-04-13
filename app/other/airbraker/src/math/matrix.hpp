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

template<std::size_t R, std::size_t C>
Matrix<R,C> matrixExpPowSeries(const Matrix<R,C> &At, std::size_t iterations){

    Matrix<R,C> id = Matrix<R,C>::Identity();

    Matrix<R,C> sum = Matrix<R,C>::Identity();
    
    Matrix<R,C> Atn = At;
    
    for (std::size_t i = 1; i <= iterations; i++){

        float overI = (1.F / (float)i);
        Matrix<R, C> scaled = Atn * overI;
        sum = sum + scaled;
        // sum = sum + (Atn * (1/(float)factorial));
        Matrix<R,C> Atn2  = Atn * At;
        Atn = Atn2 * overI;
    }
    return sum;
}


