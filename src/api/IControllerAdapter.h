#pragma once
#include <string>

namespace camera_service::api {
    class Controller; // forward declaration

    class IControllerAdapter {
    public:
        virtual ~IControllerAdapter() = default;

        virtual void setController(Controller* controller) = 0;
        virtual bool start(const std::string& port) = 0;
        virtual void stop() = 0;
        virtual void runLoop() = 0;
    };
}
