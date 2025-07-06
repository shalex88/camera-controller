#pragma once

namespace camera_service::types {
    using zoom = double;
    using focus = double;

    struct CameraLimits {
        zoom min_zoom;
        zoom max_zoom;
        focus min_focus;
        focus max_focus;
    };
}
