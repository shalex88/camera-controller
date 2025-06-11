#pragma once

#include <memory>
#include <string>
#include <stdexcept>

namespace nfov {

namespace data {
class ICamera;
}

namespace core {

class CoreException : public std::runtime_error {
public:
    explicit CoreException(const std::string& message) : std::runtime_error(message) {}
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

class Core : public ICore {
public:
    explicit Core(std::unique_ptr<data::ICamera> camera);
    ~Core() override;

    bool initialize() override;
    void shutdown() override;

    void setZoom(double zoomLevel) override;
    double getZoom() const override;

    void setFocus(double focusValue) override;
    double getFocus() const override;

private:
    std::unique_ptr<data::ICamera> m_camera;
    bool m_initialized;
};

} // namespace core
} // namespace nfov
