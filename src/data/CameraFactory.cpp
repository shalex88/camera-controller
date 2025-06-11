#include "CameraFactory.h"

#include "NfovCamera.h"
#include "WfovCamera.h"

namespace camera_service::data {
    std::unique_ptr<ICamera> CameraFactory::createCamera(const std::string& cameraType) {
        if (cameraType == "nfov") {
            return std::make_unique<NfovCamera>();
        } else if (cameraType == "wfov") {
            return std::make_unique<WfovCamera>();
        }
        throw CameraException("Unknown camera type");
    }
}
