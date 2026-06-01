#pragma once
#include <optional>
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

float dot(const Matrix<3, 1>& a, const Matrix<3, 1>& b);

template <std::size_t R, std::size_t C>
Matrix<R, C> matrixExpPowSeries(const Matrix<R, C>& At, std::size_t iterations) {

    Matrix<R, C> id = Matrix<R, C>::Identity();

    Matrix<R, C> sum = Matrix<R, C>::Identity();

    Matrix<R, C> Atn = At;

    for (std::size_t i = 1; i <= iterations; i++) {

        float overI = (1.F / (float) i);
        Matrix<R, C> scaled = Atn * overI;
        sum = sum + scaled;
        // sum = sum + (Atn * (1/(float)factorial));
        Matrix<R, C> Atn2 = Atn * At;
        Atn = Atn2 * overI;
    }
    return sum;
}

inline std::optional<Matrix<2, 2>> inv2x2(const Matrix<2, 2>& mat) {
    float a = mat.Get(0, 0);
    float b = mat.Get(0, 1);
    float c = mat.Get(1, 0);
    float d = mat.Get(1, 1);
    float det = a * d - b * c;
    if (fabs(det) < 0.0001F) {
        return std::nullopt;
    }
    Matrix<2, 2> mat2{{d, -b, -c, a}};
    return mat2 * (1.0F / det);
}
