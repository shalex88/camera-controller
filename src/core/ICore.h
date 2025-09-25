#pragma once
#include <string>
#include "common/types/Result.h"
#include "common/types/CameraTypes.h"

namespace camera_service::core {
    class CoreException final : public std::runtime_error {
    public:
        explicit CoreException(const std::string& message) : std::runtime_error(message) {
        }
    };

    class ICore {
    public:
        virtual ~ICore() = default;

        // Service lifecycle
        virtual Result<void> initialize() = 0;
        virtual Result<void> shutdown() = 0;

        // Business methods for zoom operations
        virtual Result<void> setZoom(types::zoom zoom_level) const = 0;
        virtual Result<types::zoom> getZoom() const = 0;
        virtual Result<void> goToMinZoom() const = 0;
        virtual Result<void> goToMaxZoom() const = 0;

        // Business methods for focus operations
        virtual Result<void> setFocus(types::focus focus_value) const = 0;
        virtual Result<types::focus> getFocus() const = 0;
        virtual Result<void> enableAutoFocus(bool on) const = 0;
        virtual Result<bool> isAutoFocusEnabled() const = 0;

        // Business methods for info operations
        virtual Result<types::info> getInfo() const = 0;
    };
}
