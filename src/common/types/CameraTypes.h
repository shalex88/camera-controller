#pragma once

#include <cstdint>

namespace camera_service::types {
    using zoom = uint32_t;
    using focus = uint32_t;
    using info = std::string;

    struct CameraLimits {
        zoom min_zoom;
        zoom max_zoom;
        focus min_focus;
        focus max_focus;
    };
}
