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

        virtual Result<void> initialize() = 0;
        virtual Result<void> shutdown() = 0;

        virtual Result<void> setZoom(types::zoom zoom_level) = 0;
        virtual Result<types::zoom> getZoom() const = 0;

        virtual Result<void> setFocus(types::focus focus_value) = 0;
        virtual Result<types::focus> getFocus() const = 0;
    };
}
