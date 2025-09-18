//Follows C++ guidelines
#pragma once

#include <typeindex>
#include <unordered_map>
#include <memory>
#include "common/types/Result.h"

namespace camera_service::core {
    class ICameraService {
    public:
        virtual ~ICameraService() = default;

        // Capability management
        template<typename TCapability>
        TCapability* getCapability() {
            return static_cast<TCapability*>(getCapabilityImpl(std::type_index(typeid(TCapability))));
        }

        template<typename TCapability>
        bool hasCapability() const {
            return hasCapabilityImpl(std::type_index(typeid(TCapability)));
        }

        // Service lifecycle
        virtual Result<void> initialize() = 0;
        virtual Result<void> shutdown() = 0;

    protected:
        // Non-template virtual methods for implementation
        virtual void* getCapabilityImpl(const std::type_index& type_id) = 0;
        virtual bool hasCapabilityImpl(const std::type_index& type_id) const = 0;
    };
}