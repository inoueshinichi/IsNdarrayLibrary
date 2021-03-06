#pragma once

#include <nbla/function/utils/base_transform_unary.hpp>

#include <cmath>

namespace Is
{
    namespace nbla
    {
        /**
         * @brief Elementwise r pow scalar
         * y_i = v ^ x_i
         * 
         * Inputs:
         * - N-D array.
         * 
         * Outputs:
         * - N-D array.
         * 
         * @param T Data type for computation.
         * @param val Value of the scalar.
         */
        NBLA_DEFINE_TRANSFORM_UNARY_1(RPowScalar, std::pow((T)a0, x), double)
    }
}