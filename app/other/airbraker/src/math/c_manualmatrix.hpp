#pragma once
#include <array>
#include <cstdint>
template <std::size_t R, std::size_t C, typename Scalar = float, typename Index = std::uint8_t>
class ManualMatrix {
  public:
    using S = Scalar;
    using I = Index;

    ManualMatrix() = default;
    constexpr ManualMatrix(std::array<std::array<S, C>, R> init) : vals(init) {}

    static constexpr ManualMatrix Zeros() {
        ManualMatrix m;
        std::fill(m.vals.begin(), m.vals.end(), std::array<S, C>{0});
        return m;
    }
    static constexpr ManualMatrix Identity() {
        static_assert(R == C, "ManualMatrix must be square to make a transpose");
        ManualMatrix m = Zeros();
        for (Index i = 0; i < R; i++) {
            m.vals[i][i] = 1;
        }
        return m;
    }
    static constexpr ManualMatrix Column(const std::array<Scalar, R> list) {
        static_assert(C == 1, "Column vector must be N by 1");
        ManualMatrix m = Zeros();
        for (Index i = 0; i < R; i++) {
            m.vals[i][0] = list[i];
        }
        return m;
    }

    template <std::size_t R2, std::size_t C2>
    constexpr ManualMatrix<R, C2, Scalar, Index> operator+(const ManualMatrix<R2, C2, Scalar, Index> &rhs) const {
        static_assert(C == C2 && R == R2, "Dimensions must match to add");
        ManualMatrix<R, C, Scalar, Index> outp = ManualMatrix<R, C, Scalar, Index>::Zeros();
        for (Index r = 0; r < R; r++) {
            for (Index c = 0; c < C; c++) {
                outp.vals[r][c] = this->vals[r][c] + rhs.vals[r][c];
            }
        }
        return outp;
    }
    template <std::size_t R2, std::size_t C2>
    constexpr ManualMatrix<R, C2, Scalar, Index> operator-(const ManualMatrix<R2, C2, Scalar, Index> &lhs) const {
        static_assert(C == C2 && R == R2, "Dimensions must match to subtract");
        ManualMatrix<R, C2, Scalar, Index> outp = ManualMatrix<R, C, Scalar, Index>::Zeros();
        for (Index r = 0; r < R; r++) {
            for (Index c = 0; c < C; c++) {
                outp.vals[r][c] = this->vals[r][c] - lhs.vals[r][c];
            }
        }
        return outp;
    }

    template <std::size_t R2, std::size_t C2>
    constexpr ManualMatrix<R, C2, Scalar, Index> operator*(const ManualMatrix<R2, C2, Scalar, Index> &lhs) const {
        static_assert(C == R2, "Middles must match to multiply them");
        ManualMatrix<R, C2, Scalar, Index> outp = ManualMatrix<R, C2, Scalar, Index>::Zeros();
        for (Index r = 0; r < R; r++) {
            for (Index c = 0; c < C2; c++) {
                for (Index i = 0; i < R2; i++) {
                    outp.vals[r][c] += this->vals[r][i] * lhs.vals[i][c];
                }
            }
        }
        return outp;
    }

    void Set(Index r, Index c, Scalar value) { vals[r][c] = value; }
    Scalar Get(Index r, Index c) { return vals[r][c]; }

    std::array<std::array<S, C>, R> vals = {{0}};
};

namespace ManualMatrixTests {
constexpr ManualMatrix<2, 2> testMult2x2x2() {
    constexpr ManualMatrix<2, 2> l;
    constexpr ManualMatrix<2, 2> r;
    return l * r;
}

constexpr ManualMatrix<4, 4> testMult4x2x4() {
    constexpr ManualMatrix<4, 2> l;
    constexpr ManualMatrix<2, 4> r;
    return l * r;
}

constexpr ManualMatrix<3, 9> testMult3x2x9() {
    constexpr ManualMatrix<3, 2> l;
    constexpr ManualMatrix<2, 9> r;
    return l * r;
}

constexpr ManualMatrix<4, 4> zero4x2x4 = testMult4x2x4();
constexpr ManualMatrix<3, 9> zero3x2x9 = testMult3x2x9();

} // namespace ManualMatrixTests