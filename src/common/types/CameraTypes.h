#pragma once

#include <cstdint>

namespace camera_service::types {
    using zoom = uint32_t;
    using focus = uint32_t;
    using info = std::string;

    struct ZoomRange {
        zoom min;
        zoom max;
    };

    struct FocusRange {
        focus min;
        focus max;
    };
}
