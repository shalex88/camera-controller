#pragma once
#include <cstdint>

#include "common/types/Result.h"

namespace service::infrastructure {
    class VideoChannel {
    public:
        static Result<void> initialize(uint32_t channel_num);
    };
} // namespace service::infrastructure