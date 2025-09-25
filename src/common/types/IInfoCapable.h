#pragma once

#include "common/types/CameraTypes.h"
#include "common/types/Result.h"

namespace camera_service::capabilities {
    class IInfoCapable {
    public:
        virtual ~IInfoCapable() = default;

        virtual Result<types::info> getInfo() const = 0;
    };
}