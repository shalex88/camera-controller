#pragma once

#include <memory>
#include <string>

#include "common/types/CameraCapabilities.h"
#include "common/types/Result.h"
#include "infrastructure/camera/hal/ICameraHw.h"
#include "infrastructure/camera/protocol/genicam/GenTLLoader.h"
#include "infrastructure/camera/protocol/gentl/GenTL.h"

namespace camera_service::infrastructure {
    class ItlProtocol;

    /**
     * @brief GenTL-based camera implementation using standard GenICam Transport Layer
     *
     * This camera uses the GenTL Producer library (libFpgaCXP.cti) to access
     * CoaXPress cameras through the standardized GenTL interface. It provides:
     * - Device discovery and enumeration
     * - Register access through GenApi IPort
     * - Standard acquisition control
     * - Buffer management
     *
     * Opens first interface (index 0) and first device (index 0) found.
     */
    class AdimecGenTLCamera final : public ICameraHw,
                              public capabilities::IInfoCapable {
    public:
        /**
         * @brief Construct camera with optional lens control
         * @param producer_path Path to .cti GenTL Producer library
         * @param lens_protocol Optional lens control protocol (e.g., ITL over TCP)
         */
        explicit AdimecGenTLCamera(
            std::string producer_path,
            std::unique_ptr<ItlProtocol> lens_protocol = nullptr
        );
        ~AdimecGenTLCamera() override;

        Result<void> open() override;
        Result<void> close() override;
        Result<types::info> getInfo() const override;

        // Acquisition control
        Result<void> startAcquisition();
        Result<void> stopAcquisition();

        // Access to GenApi port for advanced configuration
        GenTL::PORT_HANDLE getDevicePort() const { return device_port_; }

    private:
        std::string producer_path_;
        std::unique_ptr<ItlProtocol> lens_protocol_;

        // GenTL dynamic loader
        std::unique_ptr<GenTLLoader> gentl_;

        // GenTL handles (owned lifecycle)
        GenTL::TL_HANDLE tl_handle_{nullptr};
        GenTL::IF_HANDLE if_handle_{nullptr};
        GenTL::DEV_HANDLE dev_handle_{nullptr};
        GenTL::DS_HANDLE stream_handle_{nullptr};
        GenTL::PORT_HANDLE device_port_{nullptr};

        Result<void> initializeGenTL();
        Result<void> discoverAndOpenDevice();
        Result<void> cleanupGenTL();
    };
}
