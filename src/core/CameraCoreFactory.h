#pragma once
#include <memory>
#include <string>

#include "data/ICamera.h"
#include "core/ICore.h"

namespace camera_service::core {
    class CameraCoreFactory {
    public:
        static std::unique_ptr<ICore> createCore(const std::string& coreType, std::unique_ptr<data::ICamera> camera);
    };
}
