#pragma once

#include <memory>
#include <string>
#include <stdexcept>

namespace nfov {

namespace core {
class ICore;
}

namespace api {

class ControllerException : public std::runtime_error {
public:
    explicit ControllerException(const std::string& message) : std::runtime_error(message) {}
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

class CameraController : public ICameraController {
public:
    explicit CameraController(std::shared_ptr<core::ICore> core);
    ~CameraController() override;

    bool start() override;
    void stop() override;

    bool setZoom(double zoomLevel) override;
    double getZoom() override;

    bool setFocus(double focusValue) override;
    double getFocus() override;

private:
    std::shared_ptr<core::ICore> m_core;
    bool m_running;
};

} // namespace api
} // namespace nfov
