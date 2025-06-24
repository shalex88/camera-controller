#pragma once
#include <string>
#include <stdexcept>

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

        virtual bool initialize() = 0;
        virtual void shutdown() = 0;

        virtual void setZoom(types::zoom zoom_level) = 0;
        virtual types::zoom getZoom() const = 0;

        virtual void setFocus(types::focus focus_value) = 0;
        virtual types::focus getFocus() const = 0;
    };
}
