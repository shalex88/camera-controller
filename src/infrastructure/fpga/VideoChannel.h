#pragma once
#include <cstdint>

namespace service::infrastructure {
    class VideoChannel {
    public:
        explicit VideoChannel(uint32_t channel_num);
        ~VideoChannel() noexcept;
    };
} // namespace service::infrastructure