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