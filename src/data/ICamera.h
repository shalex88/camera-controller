#pragma once
#include <stdexcept>

namespace camera_service::data {
    class CameraException final : public std::runtime_error {
    public:
        explicit CameraException(const char* message) : std::runtime_error(message) {
        }
    };

    class ICamera {
    public:
        virtual ~ICamera() = default;

        virtual void setZoom(double zoomLevel) = 0;
        virtual double getZoom() const = 0;
        virtual void setFocus(double focusValue) = 0;
        virtual double getFocus() const = 0;
        virtual bool connect() = 0;
        virtual void disconnect() = 0;
        virtual bool isConnected() const = 0;
    };
}