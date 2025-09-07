#include "CoreFactory.h"

#include "Core.h"

namespace camera_service::core {
    std::unique_ptr<ICore> CoreFactory::createCore(
        std::unique_ptr<data::ICameraHal> camera,
        std::shared_ptr<LayerLogger> logger, const CoreConfig& config) {
        if (config.camera == "nfov" || config.camera == "wfov") {
            return std::make_unique<Core>(std::move(camera), std::move(logger));
        }
        throw std::invalid_argument("Unknown core type");
    }
}
