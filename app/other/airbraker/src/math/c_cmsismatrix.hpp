#pragma once
#include "arm_math.h"

#include <array>
#include <cstdint>

template <std::size_t R, std::size_t C>
class CMSISMatrix {
  public:
    using Scalar = float32_t;

    CMSISMatrix() { arm_mat_init_f32(&inst, R, C, data); }
    CMSISMatrix(Scalar (&arr)[R * C]) {
        for (std::size_t i = 0; i < R * C; i++) {
            data[i] = arr[i];
        }
        arm_mat_init_f32(&inst, R, C, data);
    }
    CMSISMatrix(std::array<std::array<float, C>, R> init) {
        for (std::size_t i = 0; i < R; i++) {
            for (std::size_t j = 0; j < C; j++) {
                data[(i * C) + j] = (float32_t) init[i][j];
            }
        }
        arm_mat_init_f32(&inst, R, C, data);
    }

    static CMSISMatrix Zeros() {
        CMSISMatrix m;
        for (std::size_t i = 0; i < R * C; i++) {
            m.data[i] = 0;
        }
        arm_mat_init_f32(&m.inst, R, C, m.data);
        return m;
    }
    static CMSISMatrix Identity() {
        static_assert(R == C, "Matrix must be square to make a transpose");
        CMSISMatrix m = Zeros();
        for (std::size_t i = 0; i < R; i++) {
            m.data[(i * C) + i] = 1;
        }
        return m;
    }
    static CMSISMatrix Column(const std::array<Scalar, R> list) {
        static_assert(C == 1, "Column vector must be N by 1");
        CMSISMatrix m = Zeros();
        for (std::size_t i = 0; i < R; i++) {
            m.data[i * C] = list[i];
        }
        return m;
    }

    template <std::size_t R2, std::size_t C2>
    CMSISMatrix<R, C2> operator+(const CMSISMatrix<R2, C2> &lhs) const {
        static_assert(C == C2 && R == R2, "Dimensions must match to add");
        CMSISMatrix<R, C2> outp = CMSISMatrix{};
        arm_mat_add_f32(&this->inst, &lhs.inst, &outp.inst);
        return outp;
    }
    template <std::size_t R2, std::size_t C2>
    constexpr CMSISMatrix<R, C2> operator-(const CMSISMatrix<R2, C2> &lhs) const {
        static_assert(C == C2 && R == R2, "Dimensions must match to subtract");
        CMSISMatrix<R, C2> outp = CMSISMatrix{};
        arm_mat_sub_f32(&this->inst, &lhs.inst, &outp.inst);
        return outp;
    }

    template <std::size_t R2, std::size_t C2>
    constexpr CMSISMatrix<R, C2> operator*(const CMSISMatrix<R2, C2> &lhs) const {
        static_assert(C == R2, "Middles must match to multiply them");
        CMSISMatrix<R, C2> outp = CMSISMatrix<R, C2>::Zeros();
        arm_mat_mult_f32(&this->inst, &lhs.inst, &outp.inst);

        return outp;
    }

    void Set(std::size_t r, std::size_t c, Scalar value) { data[r * C + c] = value; }
    Scalar Get(std::size_t r, std::size_t c) { return data[r * C + c]; }

    //   private:
    arm_matrix_instance_f32 inst = {};
    float32_t data[R * C] = {0};
};