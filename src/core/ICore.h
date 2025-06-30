#pragma once
#include <string>
#include "common/types/Result.h"
#include "common/types/ICameraOperations.h"

namespace camera_service::core {
    class CoreException final : public std::runtime_error {
    public:
        explicit CoreException(const std::string& message) : std::runtime_error(message) {
        }
    };

    class ICore : public api::ICameraOperations {
    public:
        ~ICore() override = default;

        virtual Result<void> initialize() = 0;
        virtual Result<void> shutdown() = 0;
    };
}
