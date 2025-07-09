#pragma once
#include <memory>
#include <string>

#include "ICameraHal.h"

namespace camera_service::data {
    class CameraFactory {
    public:
        static std::unique_ptr<ICameraHal> createCamera(const std::string& camera_type);
    };
}
