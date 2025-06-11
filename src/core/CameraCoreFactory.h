#pragma once
#include <memory>
#include <string>

#include "CameraCore.h"
#include "data/ICamera.h"

namespace camera_service::core {
    class CameraCoreFactory {
    public:
        static std::unique_ptr<ICore> createCore(const std::string& coreType, std::unique_ptr<data::ICamera> camera);
    };
}
