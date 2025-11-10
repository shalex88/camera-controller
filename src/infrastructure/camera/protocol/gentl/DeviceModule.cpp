#include <cstring>

#include "GenTLImpl.h"
#include "common/logger/Logger.h"
#include "infrastructure/camera/protocol/genicam/FpgaTransport.h"

using namespace GenTL;

namespace camera_service::infrastructure::gentl {
    DeviceModule::DeviceModule(std::string id, std::string device)
        : devicePath_(std::move(device)) {
        info_.id = std::move(id);
        info_.vendor = "Unknown";
        info_.model = "Unknown";
        info_.serialNumber = "Unknown";
        info_.version = "1.0.0";
        info_.displayName = "CoaXPress Device";
        info_.userDefinedName = "";
        info_.tlType = "CXP";
        info_.timestampFrequency = 1000000000; // 1 GHz

        initializeDevice();
        LOG_INFO("GenTL Device Module initialized: {}", info_.id);
    }

    DeviceModule::~DeviceModule() {
        if (port_) {
            delete static_cast<FpgaTransport*>(port_);
            port_ = nullptr;
        }
        LOG_INFO("GenTL Device Module closed: {}", info_.id);
    }

    GC_ERROR DeviceModule::getInfo(DEVICE_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size) {
        if (type) {
            *type = INFO_DATATYPE_STRING; // String type by default
        }

        auto copy_string = [type](const std::string& src, void* buffer, size_t* size) -> GC_ERROR {
            if (size == nullptr) {
                return GC_ERR_INVALID_PARAMETER;
            }
            const size_t required = src.length() + 1;
            if (type) {
                *type = INFO_DATATYPE_STRING;
            }
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

        auto copy_integer = [type](auto value, void* buffer, size_t* size) -> GC_ERROR {
            if (size == nullptr) {
                return GC_ERR_INVALID_PARAMETER;
            }
            constexpr size_t required = sizeof(value);
            if (type) {
                if constexpr (sizeof(value) == 2) {
                    *type = INFO_DATATYPE_UINT16;
                } else if constexpr (sizeof(value) == 4) {
                    *type = INFO_DATATYPE_UINT32;
                } else if constexpr (sizeof(value) == 8) {
                    *type = INFO_DATATYPE_UINT64;
                }
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
            if (type) {
                *type = 0;
            }
            return GC_ERR_SUCCESS;
        };

        switch (cmd) {
            case DEVICE_INFO_ID:
                return copy_string(info_.id, buffer, size);
            case DEVICE_INFO_VENDOR:
                return copy_string(info_.vendor, buffer, size);
            case DEVICE_INFO_MODEL:
                return copy_string(info_.model, buffer, size);
            case DEVICE_INFO_TLTYPE:
                return copy_string("CXP", buffer, size);
            case DEVICE_INFO_DISPLAYNAME:
                return copy_string(info_.displayName, buffer, size);
            case DEVICE_INFO_ACCESS_STATUS:
                return copy_integer(static_cast<int32_t>(DEVICE_ACCESS_STATUS_READWRITE), buffer, size);
            case DEVICE_INFO_USER_DEFINED_NAME:
                return copy_string(info_.userDefinedName, buffer, size);
            case DEVICE_INFO_SERIAL_NUMBER:
                return copy_string(info_.serialNumber, buffer, size);
            case DEVICE_INFO_VERSION:
                return copy_string(info_.version, buffer, size);
            case DEVICE_INFO_TIMESTAMP_FREQUENCY:
                return copy_integer(info_.timestampFrequency, buffer, size);
            default:
                return GC_ERR_INVALID_PARAMETER;
        }
    }

    GC_ERROR DeviceModule::getPort(PORT_HANDLE* portHandle) {
        if (portHandle == nullptr) {
            return GC_ERR_INVALID_PARAMETER;
        }

        if (port_ == nullptr) {
            return GC_ERR_NOT_INITIALIZED;
        }

        *portHandle = port_;
        return GC_ERR_SUCCESS;
    }

    GC_ERROR DeviceModule::getNumDataStreams(uint32_t* numStreams) {
        if (numStreams == nullptr) {
            return GC_ERR_INVALID_PARAMETER;
        }

        *numStreams = 1; // Single stream for now
        return GC_ERR_SUCCESS;
    }

    GC_ERROR DeviceModule::getDataStreamID(uint32_t index, char* streamID, size_t* size) {
        if (index != 0) {
            return GC_ERR_INVALID_INDEX;
        }

        const std::string id = "Stream0";
        if (size == nullptr) {
            return GC_ERR_INVALID_PARAMETER;
        }

        const size_t required = id.length() + 1;
        if (streamID == nullptr) {
            *size = required;
            return GC_ERR_SUCCESS;
        }

        if (*size < required) {
            *size = required;
            return GC_ERR_BUFFER_TOO_SMALL;
        }

        std::memcpy(streamID, id.c_str(), required);
        *size = required;
        return GC_ERR_SUCCESS;
    }

    GC_ERROR DeviceModule::openDataStream(const char* streamID, std::unique_ptr<StreamModule>& stream) {
        if (streamID == nullptr) {
            return GC_ERR_INVALID_PARAMETER;
        }

        if (std::string(streamID) != "Stream0") {
            return GC_ERR_INVALID_ID;
        }

        try {
            stream = std::make_unique<StreamModule>("Stream0", this);
            LOG_INFO("Opened data stream: {}", streamID);
            return GC_ERR_SUCCESS;
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to open stream {}: {}", streamID, e.what());
            return GC_ERR_ERROR;
        }
    }

    void DeviceModule::initializeDevice() {
        try {
            // Create FpgaTransport (IPort implementation)
            auto* transport = new FpgaTransport(devicePath_);
            port_ = transport;
            LOG_INFO("FPGA transport initialized for device {}", info_.id);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize FPGA transport: {}", e.what());
            throw;
        }
    }
}