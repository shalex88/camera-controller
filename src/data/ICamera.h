#pragma once
#include <stdexcept>

#include "common/types/CameraTypes.h"

namespace camera_service::data {
    class CameraException final : public std::runtime_error {
    public:
        explicit CameraException(const char* message) : std::runtime_error(message) {
        }
    };

    class ICamera {
    public:
        virtual ~ICamera() = default;

        virtual void setZoom(types::zoom zoom_level) = 0;
        virtual types::zoom getZoom() const = 0;
        virtual void setFocus(types::focus focus_value) = 0;
        virtual types::focus getFocus() const = 0;
        virtual bool connect() = 0;
        virtual void disconnect() = 0;
        virtual bool isConnected() const = 0;
    };
}