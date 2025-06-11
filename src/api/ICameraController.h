#pragma once
#include <string>
#include <stdexcept>

namespace camera_service::api {
    class ControllerException final : public std::runtime_error {
    public:
        explicit ControllerException(const std::string& message) : std::runtime_error(message) {
        }
    };

    class ICameraController {
    public:
        virtual ~ICameraController() = default;

        virtual bool start() = 0;
        virtual void stop() = 0;

        virtual bool setZoom(double zoomLevel) = 0;
        virtual double getZoom() = 0;

        virtual bool setFocus(double focusValue) = 0;
        virtual double getFocus() = 0;
    };
}
