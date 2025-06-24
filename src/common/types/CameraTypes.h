#pragma once

namespace camera_service::types {
    using zoom = double;
    using focus = double;

    namespace limits {
        constexpr zoom MIN_ZOOM = 1.0;
        constexpr zoom MAX_ZOOM = 10.0;
        constexpr zoom DEFAULT_ZOOM = 1.0;

        constexpr focus MIN_FOCUS = 0.0;
        constexpr focus MAX_FOCUS = 10.0;
        constexpr focus DEFAULT_FOCUS = 5.0;
    }
}
