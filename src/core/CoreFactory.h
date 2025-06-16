#pragma once
#include <memory>
#include <string>

#include "data/ICamera.h"
#include "core/ICore.h"

namespace camera_service::core {
    class CoreFactory {
    public:
        static std::unique_ptr<ICore> createCore(const std::string& core_type, std::unique_ptr<data::ICamera> camera);
    };
}
