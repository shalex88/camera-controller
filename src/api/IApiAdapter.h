#pragma once
#include <string>

namespace camera_service::api {
    class IController; // forward declaration

    class IApiAdapter {
    public:
        virtual ~IApiAdapter() = default;

        virtual void setController(IController* controller) = 0;
        virtual bool start(const std::string& port) = 0;
        virtual void stop() = 0;
        virtual void runLoop() = 0;
    };
}
