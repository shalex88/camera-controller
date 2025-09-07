#pragma once

#include "IRegisterImpl.h"
#include "RegistersMap.h"
#include <memory>

namespace camera_service::data {
    class RegistersMapManager {
    public:
        explicit RegistersMapManager(std::unique_ptr<IRegisterImpl> impl) :
            register_(std::move(impl)) {
        };

        ~RegistersMapManager() = default;

        bool setValue(REG reg, uint32_t value) const;
        uint32_t getValue(REG reg) const;
        bool resetValue(REG reg) const;
        bool clearValue(REG reg) const;
        bool setBit(REG reg, uint8_t bit_index) const;
        bool clearBit(REG reg, uint8_t bit_index) const;
        uint8_t getNibble(REG reg, uint8_t nibble_index) const;
        uint8_t setNibble(REG reg, uint8_t nibble_index, uint8_t nibble_value) const;
        bool resetAll() const;
        bool clearAll() const;

    private:
        std::unique_ptr<IRegisterImpl> register_;
        mutable std::mutex mutex_;
    };
}
