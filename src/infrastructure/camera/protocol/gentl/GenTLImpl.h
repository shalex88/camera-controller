#pragma once

#include "GenTL.h"
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <cstdint>

using namespace GenTL;

namespace camera_service::infrastructure::gentl {

// Forward declarations
class SystemModule;
class InterfaceModule;
class DeviceModule;
class StreamModule;
class BufferModule;

//-----------------------------------------------------------------------------
// Internal handle types
//-----------------------------------------------------------------------------
struct TLHandleImpl {
    std::unique_ptr<SystemModule> system;
};

struct IFHandleImpl {
    std::unique_ptr<InterfaceModule> interface;
    TL_HANDLE parent;
};

struct DevHandleImpl {
    std::unique_ptr<DeviceModule> device;
    IF_HANDLE parent;
};

struct DSHandleImpl {
    std::unique_ptr<StreamModule> stream;
    DEV_HANDLE parent;
};

struct BufferHandleImpl {
    std::unique_ptr<BufferModule> buffer;
    DS_HANDLE parent;
};

//-----------------------------------------------------------------------------
// System Module
//-----------------------------------------------------------------------------
class SystemModule {
public:
    SystemModule();
    ~SystemModule();

    GC_ERROR getInfo(TL_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size);
    GC_ERROR getNumInterfaces(uint32_t* numIfaces);
    GC_ERROR getInterfaceID(uint32_t index, char* ifaceID, size_t* size);
    GC_ERROR getInterfaceInfo(const char* ifaceID, INTERFACE_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size);
    GC_ERROR openInterface(const char* ifaceID, std::unique_ptr<InterfaceModule>& interface);
    GC_ERROR updateInterfaceList(bool8_t* changed, uint64_t timeout);

private:
    struct InterfaceInfo {
        std::string id;
        std::string displayName;
        std::string tlType;  // STRING: Transport layer type (e.g., "CXP")
    };

    std::vector<InterfaceInfo> interfaces_{};
    mutable std::mutex mutex_{};

    void discoverInterfaces();
};

//-----------------------------------------------------------------------------
// Interface Module
//-----------------------------------------------------------------------------
class InterfaceModule {
public:
    explicit InterfaceModule(std::string id);
    ~InterfaceModule();

    GC_ERROR getInfo(INTERFACE_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size);
    GC_ERROR getNumDevices(uint32_t* numDevices);
    GC_ERROR getDeviceID(uint32_t index, char* devID, size_t* size);
    GC_ERROR updateDeviceList(bool8_t* changed, uint64_t timeout);
    GC_ERROR getDeviceInfo(const char* devID, DEVICE_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size);
    GC_ERROR openDevice(const char* devID, DEVICE_ACCESS_FLAGS accessMode, std::unique_ptr<DeviceModule>& device);

private:
    struct DeviceInfo {
        std::string id;
        std::string vendor;
        std::string model;
        std::string serialNumber;
        std::string version;
        std::string displayName;
        std::string userDefinedName;
        std::string tlType;  // STRING: Transport layer type (e.g., "CXP")
        DEVICE_ACCESS_STATUS accessStatus;
        uint64_t timestampFrequency;
    };

    std::string id_{};
    std::vector<DeviceInfo> devices_{};
    mutable std::mutex mutex_{};

    void discoverDevices();
};

//-----------------------------------------------------------------------------
// Device Module
//-----------------------------------------------------------------------------
class DeviceModule {
public:
    explicit DeviceModule(std::string id, std::string device);
    ~DeviceModule();

    GC_ERROR getInfo(DEVICE_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size);
    GC_ERROR getPort(PORT_HANDLE* portHandle);
    GC_ERROR getNumDataStreams(uint32_t* numStreams);
    GC_ERROR getDataStreamID(uint32_t index, char* streamID, size_t* size);
    GC_ERROR openDataStream(const char* streamID, std::unique_ptr<StreamModule>& stream);

private:
    struct DeviceInfo {
        std::string id;
        std::string vendor;
        std::string model;
        std::string serialNumber;
        std::string version;
        std::string displayName;
        std::string userDefinedName;
        std::string tlType;  // STRING: Transport layer type (e.g., "CXP")
        uint64_t timestampFrequency;
    };

    DeviceInfo info_{};
    std::string devicePath_{};
    void* port_{nullptr}; // Points to FpgaTransport
    mutable std::mutex mutex_{};

    void initializeDevice();
};

//-----------------------------------------------------------------------------
// Stream Module
//-----------------------------------------------------------------------------
class StreamModule {
public:
    explicit StreamModule(std::string id, DeviceModule* parent);
    ~StreamModule();

    GC_ERROR getInfo(STREAM_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size);
    GC_ERROR announceBuffer(void* buffer, size_t size, void* privateData, std::unique_ptr<BufferModule>& bufferModule);
    GC_ERROR allocAndAnnounceBuffer(size_t size, void* privateData, std::unique_ptr<BufferModule>& bufferModule);
    GC_ERROR flushQueue(ACQ_QUEUE_TYPE operation);
    GC_ERROR startAcquisition(ACQ_START_FLAGS flags, uint64_t numFrames);
    GC_ERROR stopAcquisition(ACQ_STOP_FLAGS flags);
    GC_ERROR getBufferID(uint32_t index, BUFFER_HANDLE* bufferHandle);
    GC_ERROR revokeBuffer(BufferModule* bufferModule, void** buffer, void** privateData);
    GC_ERROR queueBuffer(BufferModule* bufferModule);
    GC_ERROR getBufferInfo(BufferModule* bufferModule, BUFFER_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size);

private:
    std::string id_{};
    DeviceModule* parent_{nullptr};
    bool isGrabbing_{false};
    uint64_t payloadSize_{0};
    uint64_t numDelivered_{0};
    uint64_t numUnderrun_{0};
    uint64_t numAnnounced_{0};
    uint64_t numQueued_{0};
    uint64_t numAwaitDelivery_{0};
    uint64_t numStarted_{0};
    std::vector<BufferModule*> buffers_{};
    mutable std::mutex mutex_{};
};

//-----------------------------------------------------------------------------
// Buffer Module
//-----------------------------------------------------------------------------
class BufferModule {
public:
    explicit BufferModule(void* userBuffer, size_t size, void* privateData);
    explicit BufferModule(size_t size, void* privateData);
    ~BufferModule();

    GC_ERROR getInfo(BUFFER_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size);
    void* getBase() { return base_; }
    void* getPrivateData() { return privateData_; }
    bool isOwned() const { return owned_; }

private:
    void* base_{nullptr};
    size_t size_{0};
    void* privateData_{nullptr};
    bool owned_{false};
    bool isQueued_{false};
    bool isAcquiring_{false};
    bool isIncomplete_{false};
    uint64_t timestamp_{0};
    size_t sizeFilled_{0};
    uint64_t frameId_{0};
};

} // namespace camera_service::infrastructure::gentl
