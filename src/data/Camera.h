#pragma once

#include <string>
#include <memory>
#include <stdexcept>

namespace nfov {
namespace data {

class CameraException : public std::runtime_error {
public:
    explicit CameraException(const std::string& message) : std::runtime_error(message) {}
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

class Camera : public ICamera {
public:
    Camera();
    ~Camera() override;

    void setZoom(double zoomLevel) override;
    double getZoom() const override;

    void setFocus(double focusValue) override;
    double getFocus() const override;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

private:
    double m_zoomLevel;
    double m_focusValue;
    bool m_connected;
};

} // namespace data
} // namespace nfov
