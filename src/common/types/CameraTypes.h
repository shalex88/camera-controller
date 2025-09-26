#pragma once

#include <string>

namespace camera_service::types {
    using zoom = uint32_t;
    using focus = uint32_t;
    using info = std::string;

    inline zoom kMinNormalizedZoom = 0;
    inline zoom kMaxNormalizedZoom = 100;

    inline focus kMinNormalizedFocus = 0;
    inline focus kMaxNormalizedFocus = 100;

    struct ZoomRange {
        zoom min;
        zoom max;
    };

    struct FocusRange {
        focus min;
        focus max;
    };
}
