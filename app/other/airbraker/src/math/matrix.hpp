#ifdef CONFIG_CMSIS_DSP_MATRIX
#include "cmsismatrix.hpp"

template <std::size_t R, std::size_t C>
using Matrix = CMSISMatrix<R, C>;

#else

#include "manualmatrix.hpp"
template <std::size_t R, std::size_t C>
using Matrix = ManualMatrix<R, C, float>;

#endif