#pragma once

#include <nbla/function/utils/base_transform_binary.hpp>

namespace Is
{
    namespace nbla
    {
        NBLA_DEFINE_TRANSFORM_BINARY(Equal, x0 == x1)
    }
}