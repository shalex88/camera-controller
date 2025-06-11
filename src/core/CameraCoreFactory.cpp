#include "CameraCoreFactory.h"

#include "CameraCore.h"

namespace camera_service::core {
    std::unique_ptr<ICore> CameraCoreFactory::createCore(const std::string& coreType,
        std::unique_ptr<data::ICamera> camera) {
        if (true) {
            return std::make_unique<CameraCore>(std::move(camera));
        }
        throw CoreException("Unknown core type");
    }
}
