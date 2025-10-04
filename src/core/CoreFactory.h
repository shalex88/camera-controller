#pragma once
#include <memory>

#include "common/Config/ConfigManager.h"
#include "../infrastructure/camera/hal/ICameraHal.h"
#include "core/ICore.h"
#include "common/Logger/Logger.h"

namespace camera_service::core {
    class CoreFactory {
    public:
        static std::unique_ptr<ICore> createCore(
            std::unique_ptr<data::ICameraHal> camera,
            std::shared_ptr<LayerLogger> logger, const CoreConfig& config);
    };
}
