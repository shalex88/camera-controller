#pragma once
#include <memory>
#include <string>

#include "data/ICameraHal.h"
#include "core/ICore.h"

namespace camera_service::core {
    class CoreFactory {
    public:
        static std::unique_ptr<ICore> createCore(const std::string& core_type, std::unique_ptr<data::ICameraHal> camera);
    };
}
