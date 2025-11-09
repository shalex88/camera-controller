#include "GenTLImpl.h"
#include "common/logger/Logger.h"
#include <cstring>
#include <algorithm>
#include <cstdlib>

using namespace GenTL;

namespace camera_service::infrastructure::gentl {

//-----------------------------------------------------------------------------
// StreamModule Implementation
//-----------------------------------------------------------------------------
StreamModule::StreamModule(std::string id, DeviceModule* parent)
    : id_(std::move(id)), parent_(parent) {

    // Default payload size for TMX5 (example: 2048x2048x2 bytes)
    payloadSize_ = 2048 * 2048 * 2;

    LOG_INFO("GenTL Stream Module initialized: {}", id_);
}

StreamModule::~StreamModule() {
    if (isGrabbing_) {
        stopAcquisition(ACQ_STOP_FLAGS_KILL);
    }
    LOG_INFO("GenTL Stream Module closed: {}", id_);
}

GC_ERROR StreamModule::getInfo(STREAM_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size) {
    auto copyInteger = [type](auto value, void* buffer, size_t* size) -> GC_ERROR {
        if (size == nullptr) return GC_ERR_INVALID_PARAMETER;
        constexpr size_t required = sizeof(value);
        if (type) {
            if constexpr (sizeof(value) == 2) *type = INFO_DATATYPE_UINT16;
            else if constexpr (sizeof(value) == 4) *type = INFO_DATATYPE_UINT32;
            else if constexpr (sizeof(value) == 8) *type = INFO_DATATYPE_UINT64;
        }
        if (buffer == nullptr) {
            *size = required;
            return GC_ERR_SUCCESS;
        }
        if (*size < required) {
            *size = required;
            return GC_ERR_BUFFER_TOO_SMALL;
        }
        std::memcpy(buffer, &value, required);
        *size = required;
        return GC_ERR_SUCCESS;
    };

    auto copyString = [type](const std::string& src, void* buffer, size_t* size) -> GC_ERROR {
        if (size == nullptr) return GC_ERR_INVALID_PARAMETER;
        const size_t required = src.length() + 1;
        if (type) *type = INFO_DATATYPE_STRING;
        if (buffer == nullptr) {
            *size = required;
            return GC_ERR_SUCCESS;
        }
        if (*size < required) {
            *size = required;
            return GC_ERR_BUFFER_TOO_SMALL;
        }
        std::memcpy(buffer, src.c_str(), required);
        *size = required;
        return GC_ERR_SUCCESS;
    };

    std::scoped_lock lock(mutex_);

    switch (cmd) {
        case STREAM_INFO_ID:
            return copyString(id_, buffer, size);
        case STREAM_INFO_NUM_DELIVERED:
            return copyInteger(numDelivered_, buffer, size);
        case STREAM_INFO_NUM_UNDERRUN:
            return copyInteger(numUnderrun_, buffer, size);
        case STREAM_INFO_NUM_ANNOUNCED:
            return copyInteger(numAnnounced_, buffer, size);
        case STREAM_INFO_NUM_QUEUED:
            return copyInteger(numQueued_, buffer, size);
        case STREAM_INFO_NUM_AWAIT_DELIVERY:
            return copyInteger(numAwaitDelivery_, buffer, size);
        case STREAM_INFO_NUM_STARTED:
            return copyInteger(numStarted_, buffer, size);
        case STREAM_INFO_PAYLOAD_SIZE:
            return copyInteger(payloadSize_, buffer, size);
        case STREAM_INFO_IS_GRABBING:
            return copyInteger(static_cast<bool8_t>(isGrabbing_), buffer, size);
        case STREAM_INFO_DEFINES_PAYLOADSIZE:
            return copyInteger(static_cast<bool8_t>(true), buffer, size);
        case STREAM_INFO_TLTYPE:
            return copyString("CXP", buffer, size);
        case STREAM_INFO_NUM_CHUNKS_MAX:
            return copyInteger(static_cast<uint32_t>(0), buffer, size);
        case STREAM_INFO_BUF_ANNOUNCE_MIN:
            return copyInteger(static_cast<uint32_t>(1), buffer, size);
        case STREAM_INFO_BUF_ALIGNMENT:
            return copyInteger(static_cast<uint32_t>(4096), buffer, size); // Page aligned
        default:
            return GC_ERR_INVALID_PARAMETER;
    }
}

GC_ERROR StreamModule::announceBuffer(void* buffer, size_t size, void* privateData, std::unique_ptr<BufferModule>& bufferModule) {
    if (buffer == nullptr || size == 0) {
        return GC_ERR_INVALID_PARAMETER;
    }

    std::scoped_lock lock(mutex_);

    try {
        bufferModule = std::make_unique<BufferModule>(buffer, size, privateData);
        buffers_.push_back(bufferModule.get());
        numAnnounced_++;
        LOG_DEBUG("Announced buffer: {} bytes at {}", size, buffer);
        return GC_ERR_SUCCESS;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to announce buffer: {}", e.what());
        return GC_ERR_OUT_OF_MEMORY;
    }
}

GC_ERROR StreamModule::allocAndAnnounceBuffer(size_t size, void* privateData, std::unique_ptr<BufferModule>& bufferModule) {
    if (size == 0) {
        return GC_ERR_INVALID_PARAMETER;
    }

    std::scoped_lock lock(mutex_);

    try {
        bufferModule = std::make_unique<BufferModule>(size, privateData);
        buffers_.push_back(bufferModule.get());
        numAnnounced_++;
        LOG_DEBUG("Allocated and announced buffer: {} bytes", size);
        return GC_ERR_SUCCESS;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to allocate buffer: {}", e.what());
        return GC_ERR_OUT_OF_MEMORY;
    }
}

GC_ERROR StreamModule::flushQueue(ACQ_QUEUE_TYPE operation) {
    std::scoped_lock lock(mutex_);

    switch (operation) {
        case ACQ_QUEUE_INPUT_TO_OUTPUT:
        case ACQ_QUEUE_OUTPUT_DISCARD:
        case ACQ_QUEUE_ALL_DISCARD:
        case ACQ_QUEUE_UNQUEUED_TO_INPUT:
        case ACQ_QUEUE_ALL_TO_INPUT:
            numQueued_ = 0;
            numAwaitDelivery_ = 0;
            LOG_DEBUG("Flushed queue: operation {}", static_cast<int>(operation));
            return GC_ERR_SUCCESS;
        default:
            return GC_ERR_INVALID_PARAMETER;
    }
}

GC_ERROR StreamModule::startAcquisition(ACQ_START_FLAGS flags, uint64_t numFrames) {
    (void)flags;
    (void)numFrames;

    std::scoped_lock lock(mutex_);

    if (isGrabbing_) {
        return GC_ERR_RESOURCE_IN_USE;
    }

    if (buffers_.empty()) {
        LOG_ERROR("Cannot start acquisition: no buffers announced");
        return GC_ERR_INVALID_BUFFER;
    }

    isGrabbing_ = true;
    numStarted_++;
    LOG_INFO("Started acquisition on stream {}", id_);
    return GC_ERR_SUCCESS;
}

GC_ERROR StreamModule::stopAcquisition(ACQ_STOP_FLAGS flags) {
    (void)flags;

    std::scoped_lock lock(mutex_);

    if (!isGrabbing_) {
        return GC_ERR_ABORT;
    }

    isGrabbing_ = false;
    LOG_INFO("Stopped acquisition on stream {}", id_);
    return GC_ERR_SUCCESS;
}

GC_ERROR StreamModule::getBufferID(uint32_t index, BUFFER_HANDLE* bufferHandle) {
    if (bufferHandle == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    std::scoped_lock lock(mutex_);

    if (index >= buffers_.size()) {
        return GC_ERR_INVALID_INDEX;
    }

    // Note: This is unsafe casting for demo; in production use proper handle mapping
    *bufferHandle = reinterpret_cast<BUFFER_HANDLE>(buffers_[index]);
    return GC_ERR_SUCCESS;
}

GC_ERROR StreamModule::revokeBuffer(BufferModule* bufferModule, void** buffer, void** privateData) {
    if (bufferModule == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    std::scoped_lock lock(mutex_);

    auto it = std::find(buffers_.begin(), buffers_.end(), bufferModule);
    if (it == buffers_.end()) {
        return GC_ERR_INVALID_HANDLE;
    }

    if (buffer) {
        *buffer = bufferModule->getBase();
    }
    if (privateData) {
        *privateData = bufferModule->getPrivateData();
    }

    buffers_.erase(it);
    numAnnounced_--;

    LOG_DEBUG("Revoked buffer at {}", bufferModule->getBase());
    return GC_ERR_SUCCESS;
}

GC_ERROR StreamModule::queueBuffer(BufferModule* bufferModule) {
    if (bufferModule == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    std::scoped_lock lock(mutex_);

    auto it = std::find(buffers_.begin(), buffers_.end(), bufferModule);
    if (it == buffers_.end()) {
        return GC_ERR_INVALID_HANDLE;
    }

    numQueued_++;
    LOG_TRACE("Queued buffer at {}", bufferModule->getBase());
    return GC_ERR_SUCCESS;
}

GC_ERROR StreamModule::getBufferInfo(BufferModule* bufferModule, BUFFER_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size) {
    if (bufferModule == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return bufferModule->getInfo(cmd, type, buffer, size);
}

//-----------------------------------------------------------------------------
// BufferModule Implementation
//-----------------------------------------------------------------------------
BufferModule::BufferModule(void* userBuffer, size_t size, void* privateData)
    : base_(userBuffer), size_(size), privateData_(privateData), owned_(false) {
    LOG_TRACE("Buffer module created (user-provided) at {}, {} bytes", base_, size_);
}

BufferModule::BufferModule(size_t size, void* privateData)
    : size_(size), privateData_(privateData), owned_(true) {

    // Allocate aligned memory
    if (posix_memalign(&base_, 4096, size) != 0) {
        throw std::bad_alloc();
    }

    LOG_TRACE("Buffer module created (allocated) at {}, {} bytes", base_, size_);
}

BufferModule::~BufferModule() {
    if (owned_ && base_) {
        std::free(base_);
    }
    LOG_TRACE("Buffer module destroyed");
}

GC_ERROR BufferModule::getInfo(BUFFER_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size) {
    auto copyInteger = [type](auto value, void* buffer, size_t* size) -> GC_ERROR {
        if (size == nullptr) return GC_ERR_INVALID_PARAMETER;
        constexpr size_t required = sizeof(value);
        if (type) {
            if constexpr (sizeof(value) == 1) *type = INFO_DATATYPE_BOOL8;
            else if constexpr (sizeof(value) == 2) *type = INFO_DATATYPE_UINT16;
            else if constexpr (sizeof(value) == 4) *type = INFO_DATATYPE_UINT32;
            else if constexpr (sizeof(value) == 8) *type = INFO_DATATYPE_UINT64;
        }
        if (buffer == nullptr) {
            *size = required;
            return GC_ERR_SUCCESS;
        }
        if (*size < required) {
            *size = required;
            return GC_ERR_BUFFER_TOO_SMALL;
        }
        std::memcpy(buffer, &value, required);
        *size = required;
        return GC_ERR_SUCCESS;
    };

    auto copyPointer = [type](void* value, void* buffer, size_t* size) -> GC_ERROR {
        if (size == nullptr) return GC_ERR_INVALID_PARAMETER;
        constexpr size_t required = sizeof(void*);
        if (type) *type = INFO_DATATYPE_PTR;
        if (buffer == nullptr) {
            *size = required;
            return GC_ERR_SUCCESS;
        }
        if (*size < required) {
            *size = required;
            return GC_ERR_BUFFER_TOO_SMALL;
        }
        std::memcpy(buffer, &value, required);
        *size = required;
        return GC_ERR_SUCCESS;
    };

    switch (cmd) {
        case BUFFER_INFO_BASE:
            return copyPointer(base_, buffer, size);
        case BUFFER_INFO_SIZE:
            return copyInteger(size_, buffer, size);
        case BUFFER_INFO_USER_PTR:
            return copyPointer(privateData_, buffer, size);
        case BUFFER_INFO_TIMESTAMP:
            return copyInteger(timestamp_, buffer, size);
        case BUFFER_INFO_NEW_DATA:
            return copyInteger(static_cast<bool8_t>(sizeFilled_ > 0), buffer, size);
        case BUFFER_INFO_IS_QUEUED:
            return copyInteger(static_cast<bool8_t>(isQueued_), buffer, size);
        case BUFFER_INFO_IS_ACQUIRING:
            return copyInteger(static_cast<bool8_t>(isAcquiring_), buffer, size);
        case BUFFER_INFO_IS_INCOMPLETE:
            return copyInteger(static_cast<bool8_t>(isIncomplete_), buffer, size);
        case BUFFER_INFO_SIZE_FILLED:
            return copyInteger(sizeFilled_, buffer, size);
        case BUFFER_INFO_FRAMEID:
            return copyInteger(frameId_, buffer, size);
        case BUFFER_INFO_IMAGEPRESENT:
            return copyInteger(static_cast<bool8_t>(true), buffer, size);
        default:
            return GC_ERR_NOT_IMPLEMENTED;
    }
}

} // namespace camera_service::infrastructure::gentl
