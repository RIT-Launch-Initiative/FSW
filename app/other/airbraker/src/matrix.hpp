#include <array>
#include <cstdint>

template <std::size_t R, std::size_t C, typename Scalar = float>
class Matrix {
  public:
    using S = Scalar;

    static constexpr Matrix Zeros() {
        Matrix m;
        std::fill(m.vals.begin(), m.vals.end(), std::array<S, C>{0});
        return m;
    }
    static constexpr Matrix Identity() {
        static_assert(R == C, "Matrix must be square to make a transpose");
        Matrix m = Zeros();
        for (std::size_t i = 0; i < R; i++) {
            m.vals[i][i] = 1;
        }
        return m;
    }

    template <std::size_t R2, std::size_t C2>
    constexpr Matrix<R, C2, Scalar> operator*(const Matrix<R2, C2> &lhs) const {
        static_assert(C == R2, "Middles must match to multiply them");
        Matrix<R, C2> outp = Matrix<R, C2>::Zeros();
        for (std::size_t r = 0; r < R; r++) {
            for (std::size_t c = 0; c < C2; c++) {
                for (std::size_t i = 0; i < R2; i++) {
                    outp.vals[r][c] = this->vals[r][i] * lhs.vals[i][c];
                }
            }
        }
        return outp;
    }

    //   private:
    std::array<std::array<S, C>, R> vals = {{0}};
};

constexpr Matrix<2,2> testMult2x2x2(){
    constexpr Matrix<2,2> l;
    constexpr Matrix<2,2> r;
    return l * r;
}


constexpr Matrix<4,4> testMult4x2x4(){
    constexpr Matrix<4,2> l;
    constexpr Matrix<2,4> r;
    return l * r;
}

constexpr Matrix<3,9> testMult3x2x9(){
    constexpr Matrix<3,2> l;
    constexpr Matrix<2,9> r;
    return l * r;
}

constexpr Matrix<4,4> zero4x2x4 = testMult4x2x4();
constexpr Matrix<3,9> zero3x2x9 = testMult3x2x9();