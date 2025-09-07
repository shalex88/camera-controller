#pragma once

#include <cstdint>

namespace camera_service::data {
    class IRegisterImpl {
    public:
        virtual ~IRegisterImpl() = default;
        virtual bool set(uint32_t address, uint32_t value) = 0;
        virtual bool get(uint32_t address, uint32_t& value) const = 0;
    };
}