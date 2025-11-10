#include <algorithm>
#include <cstring>

#include "GenTLImpl.h"
#include "common/logger/Logger.h"

using namespace GenTL;

namespace camera_service::infrastructure::gentl {
    namespace {
        constexpr auto TL_ID = "FpgaCXP";
        constexpr auto TL_VENDOR = "CameraService";
        constexpr auto TL_MODEL = "FPGA CoaXPress Transport Layer";
        constexpr auto TL_VERSION = "1.0.0";
        constexpr auto TL_NAME = "FpgaCXPProducer";
        constexpr auto TL_PATHNAME = "/usr/lib/genicam/gentl/FpgaCXP.cti";
        constexpr auto TL_DISPLAYNAME = "FPGA CoaXPress Producer";
        constexpr uint32_t TL_GENTL_VER_MAJOR = 1;
        constexpr uint32_t TL_GENTL_VER_MINOR = 6;

        // Helper to copy string info
        GC_ERROR copyStringInfo(const std::string& src, void* buffer, size_t* size) {
            if (size == nullptr) {
                return GC_ERR_INVALID_PARAMETER;
            }

            const size_t required = src.length() + 1;
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
        }

        // Helper to copy integer info
        template <typename T>
        GC_ERROR copyIntegerInfo(T value, INFO_DATATYPE* type, void* buffer, size_t* size) {
            if (size == nullptr) {
                return GC_ERR_INVALID_PARAMETER;
            }

            constexpr size_t required = sizeof(T);
            if (buffer == nullptr) {
                *size = required;
                if (type) {
                    if (sizeof(T) == 2) {
                        *type = INFO_DATATYPE_UINT16;
                    } else if (sizeof(T) == 4) {
                        *type = INFO_DATATYPE_UINT32;
                    } else if (sizeof(T) == 8) {
                        *type = INFO_DATATYPE_UINT64;
                    } else {
                        *type = INFO_DATATYPE_UNKNOWN;
                    }
                }
                return GC_ERR_SUCCESS;
            }

            if (*size < required) {
                *size = required;
                return GC_ERR_BUFFER_TOO_SMALL;
            }

            std::memcpy(buffer, &value, required);
            *size = required;
            if (type) {
                if (sizeof(T) == 2) {
                    *type = INFO_DATATYPE_UINT16;
                } else if (sizeof(T) == 4) {
                    *type = INFO_DATATYPE_UINT32;
                } else if (sizeof(T) == 8) {
                    *type = INFO_DATATYPE_UINT64;
                } else {
                    *type = INFO_DATATYPE_UNKNOWN;
                }
            }
            return GC_ERR_SUCCESS;
        }
    }

    //-----------------------------------------------------------------------------
    // SystemModule Implementation
    //-----------------------------------------------------------------------------
    SystemModule::SystemModule() {
        discoverInterfaces();
        LOG_DEBUG("GenTL System Module initialized");
    }

    SystemModule::~SystemModule() {
        LOG_DEBUG("GenTL System Module closed");
    }

    void SystemModule::discoverInterfaces() {
        std::scoped_lock lock(mutex_);
        interfaces_.clear();

        // Add FPGA CXP interface
        InterfaceInfo info;
        info.id = "FpgaCXP0";
        info.displayName = "FPGA CoaXPress Interface 0";
        info.tlType = "CXP";
        interfaces_.push_back(info);
    }

    GC_ERROR SystemModule::getInfo(TL_INFO_CMD cmd, int32_t* type, void* buffer, size_t* size) {
        if (type)
            *type = 1; // String type by default

        switch (cmd) {
            case TL_INFO_ID:
                return copyStringInfo(TL_ID, buffer, size);
            case TL_INFO_VENDOR:
                return copyStringInfo(TL_VENDOR, buffer, size);
            case TL_INFO_MODEL:
                return copyStringInfo(TL_MODEL, buffer, size);
            case TL_INFO_VERSION:
                return copyStringInfo(TL_VERSION, buffer, size);
            case TL_INFO_TLTYPE:
                return copyStringInfo("CXP", buffer, size);
            case TL_INFO_NAME:
                return copyStringInfo(TL_NAME, buffer, size);
            case TL_INFO_PATHNAME:
                return copyStringInfo(TL_PATHNAME, buffer, size);
            case TL_INFO_DISPLAYNAME:
                return copyStringInfo(TL_DISPLAYNAME, buffer, size);
            case TL_INFO_CHAR_ENCODING:
                return copyStringInfo("ASCII", buffer, size);
            case TL_INFO_GENTL_VER_MAJOR:
                return copyIntegerInfo(TL_GENTL_VER_MAJOR, type, buffer, size);
            case TL_INFO_GENTL_VER_MINOR:
                return copyIntegerInfo(TL_GENTL_VER_MINOR, type, buffer, size);
            default:
                return GC_ERR_INVALID_PARAMETER;
        }
    }

    GC_ERROR SystemModule::getNumInterfaces(uint32_t* numIfaces) {
        if (numIfaces == nullptr) {
            return GC_ERR_INVALID_PARAMETER;
        }

        std::scoped_lock lock(mutex_);
        *numIfaces = static_cast<uint32_t>(interfaces_.size());
        return GC_ERR_SUCCESS;
    }

    GC_ERROR SystemModule::getInterfaceID(uint32_t index, char* ifaceID, size_t* size) {
        std::scoped_lock lock(mutex_);

        if (index >= interfaces_.size()) {
            return GC_ERR_INVALID_INDEX;
        }

        return copyStringInfo(interfaces_[index].id, ifaceID, size);
    }

    GC_ERROR SystemModule::getInterfaceInfo(const char* ifaceID, INTERFACE_INFO_CMD cmd, int32_t* type, void* buffer,
                                            size_t* size) {
        if (ifaceID == nullptr) {
            return GC_ERR_INVALID_PARAMETER;
        }

        std::scoped_lock lock(mutex_);

        const auto it = std::ranges::find_if(interfaces_, [ifaceID](const InterfaceInfo& info) {
            return info.id == ifaceID;
        });

        if (it == interfaces_.end()) {
            return GC_ERR_INVALID_ID;
        }

        if (type) {
            *type = 1; // String type by default
        }

        switch (cmd) {
            case INTERFACE_INFO_ID:
                return copyStringInfo(it->id, buffer, size);
            case INTERFACE_INFO_DISPLAYNAME:
                return copyStringInfo(it->displayName, buffer, size);
            case INTERFACE_INFO_TLTYPE:
                return copyStringInfo("CXP", buffer, size);
            default:
                return GC_ERR_INVALID_PARAMETER;
        }
    }

    GC_ERROR SystemModule::openInterface(const char* ifaceID, std::unique_ptr<InterfaceModule>& interface) {
        if (ifaceID == nullptr) {
            return GC_ERR_INVALID_PARAMETER;
        }

        std::scoped_lock lock(mutex_);

        const auto it = std::ranges::find_if(interfaces_, [ifaceID](const InterfaceInfo& info) {
            return info.id == ifaceID;
        });

        if (it == interfaces_.end()) {
            return GC_ERR_INVALID_ID;
        }

        try {
            interface = std::make_unique<InterfaceModule>(it->id);
            LOG_DEBUG("Opened interface: {}", it->id);
            return GC_ERR_SUCCESS;
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to open interface {}: {}", ifaceID, e.what());
            return GC_ERR_ERROR;
        }
    }

    GC_ERROR SystemModule::updateInterfaceList(bool8_t* changed, uint64_t timeout) {
        (void)timeout; // Not used for now
        std::scoped_lock lock(mutex_);

        const size_t oldSize = interfaces_.size();
        discoverInterfaces();

        if (changed) {
            *changed = (interfaces_.size() != oldSize) ? 1 : 0;
        }

        return GC_ERR_SUCCESS;
    }

    //-----------------------------------------------------------------------------
    // InterfaceModule Implementation
    //-----------------------------------------------------------------------------
    InterfaceModule::InterfaceModule(std::string id)
        : id_(std::move(id)) {
        discoverDevices();
        LOG_DEBUG("GenTL Interface Module initialized: {}", id_);
    }

    InterfaceModule::~InterfaceModule() {
        LOG_DEBUG("GenTL Interface Module closed: {}", id_);
    }

    void InterfaceModule::discoverDevices() {
        std::scoped_lock lock(mutex_);
        devices_.clear();

        // For now, single device at /dev/mem
        DeviceInfo info;
        info.id = "CXPCamera0";
        info.vendor = "Adimec";
        info.model = "TMX5";
        info.serialNumber = "00000001";
        info.version = "1.0.0";
        info.displayName = "CoaXPress Camera 0";
        info.userDefinedName = "NFOV Camera";
        info.tlType = "CXP";
        info.accessStatus = DEVICE_ACCESS_STATUS_READWRITE;
        info.timestampFrequency = 1000000000; // 1 GHz

        devices_.push_back(info);
    }

    GC_ERROR InterfaceModule::getInfo(INTERFACE_INFO_CMD cmd, int32_t* type, void* buffer, size_t* size) {
        if (type)
            *type = 1; // String type

        switch (cmd) {
            case INTERFACE_INFO_ID:
                return copyStringInfo(id_, buffer, size);
            case INTERFACE_INFO_DISPLAYNAME:
                return copyStringInfo("FPGA CoaXPress Interface", buffer, size);
            case INTERFACE_INFO_TLTYPE:
                return copyStringInfo("CXP", buffer, size);
            default:
                return GC_ERR_INVALID_PARAMETER;
        }
    }

    GC_ERROR InterfaceModule::getNumDevices(uint32_t* numDevices) {
        if (numDevices == nullptr) {
            return GC_ERR_INVALID_PARAMETER;
        }

        std::scoped_lock lock(mutex_);
        *numDevices = static_cast<uint32_t>(devices_.size());
        return GC_ERR_SUCCESS;
    }

    GC_ERROR InterfaceModule::getDeviceID(uint32_t index, char* devID, size_t* size) {
        std::scoped_lock lock(mutex_);

        if (index >= devices_.size()) {
            return GC_ERR_INVALID_INDEX;
        }

        return copyStringInfo(devices_[index].id, devID, size);
    }

    GC_ERROR InterfaceModule::updateDeviceList(bool8_t* changed, uint64_t timeout) {
        (void)timeout;
        std::scoped_lock lock(mutex_);

        const size_t oldSize = devices_.size();
        discoverDevices();

        if (changed) {
            *changed = (devices_.size() != oldSize) ? 1 : 0;
        }

        return GC_ERR_SUCCESS;
    }

    GC_ERROR InterfaceModule::getDeviceInfo(const char* devID, DEVICE_INFO_CMD cmd, int32_t* type, void* buffer,
                                            size_t* size) {
        if (devID == nullptr) {
            return GC_ERR_INVALID_PARAMETER;
        }

        std::scoped_lock lock(mutex_);

        const auto it = std::ranges::find_if(devices_, [devID](const DeviceInfo& info) {
            return info.id == devID;
        });

        if (it == devices_.end()) {
            return GC_ERR_INVALID_ID;
        }

        if (type) {
            *type = 1; // String type by default
        }

        switch (cmd) {
            case DEVICE_INFO_ID:
                return copyStringInfo(it->id, buffer, size);
            case DEVICE_INFO_VENDOR:
                return copyStringInfo(it->vendor, buffer, size);
            case DEVICE_INFO_MODEL:
                return copyStringInfo(it->model, buffer, size);
            case DEVICE_INFO_TLTYPE:
                return copyStringInfo("CXP", buffer, size);
            case DEVICE_INFO_DISPLAYNAME:
                return copyStringInfo(it->displayName, buffer, size);
            case DEVICE_INFO_ACCESS_STATUS:
                return copyIntegerInfo(static_cast<int32_t>(it->accessStatus), type, buffer, size);
            case DEVICE_INFO_USER_DEFINED_NAME:
                return copyStringInfo(it->userDefinedName, buffer, size);
            case DEVICE_INFO_SERIAL_NUMBER:
                return copyStringInfo(it->serialNumber, buffer, size);
            case DEVICE_INFO_VERSION:
                return copyStringInfo(it->version, buffer, size);
            case DEVICE_INFO_TIMESTAMP_FREQUENCY:
                return copyIntegerInfo(it->timestampFrequency, type, buffer, size);
            default:
                return GC_ERR_INVALID_PARAMETER;
        }
    }

    GC_ERROR InterfaceModule::openDevice(const char* devID, DEVICE_ACCESS_FLAGS accessMode,
                                         std::unique_ptr<DeviceModule>& device) {
        if (devID == nullptr) {
            return GC_ERR_INVALID_PARAMETER;
        }

        std::scoped_lock lock(mutex_);

        const auto it = std::ranges::find_if(devices_, [devID](const DeviceInfo& info) {
            return info.id == devID;
        });

        if (it == devices_.end()) {
            return GC_ERR_INVALID_ID;
        }

        if (accessMode != DEVICE_ACCESS_READONLY && accessMode != DEVICE_ACCESS_CONTROL && accessMode !=
            DEVICE_ACCESS_EXCLUSIVE) {
            return GC_ERR_ACCESS_DENIED;
        }

        try {
            device = std::make_unique<DeviceModule>(it->id, "/dev/mem");
            LOG_DEBUG("Opened device: {}", it->id);
            return GC_ERR_SUCCESS;
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to open device {}: {}", devID, e.what());
            return GC_ERR_ERROR;
        }
    }
}