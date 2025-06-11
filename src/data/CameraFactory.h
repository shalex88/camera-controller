#pragma once
#include <memory>
#include <string>

#include "ICamera.h"

namespace camera_service::data {
    class CameraFactory {
    public:
        static std::unique_ptr<ICamera> createCamera(const std::string& cameraType);
    };
}
