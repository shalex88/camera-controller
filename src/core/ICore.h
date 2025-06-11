#pragma once
#include <string>
#include <stdexcept>

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

        virtual void setZoom(double zoomLevel) = 0;
        virtual double getZoom() const = 0;

        virtual void setFocus(double focusValue) = 0;
        virtual double getFocus() const = 0;
    };
}
