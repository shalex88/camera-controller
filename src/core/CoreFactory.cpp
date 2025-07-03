#include "CoreFactory.h"

#include "Core.h"

namespace camera_service::core {
    std::unique_ptr<ICore> CoreFactory::createCore(const std::string& core_type,
        std::unique_ptr<data::ICamera> camera) {
        if (core_type == "nfov" || core_type == "wfov") {
            return std::make_unique<Core>(std::move(camera));
        }
        throw std::invalid_argument("Unknown core type");
    }
}
