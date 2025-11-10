#include "GenTL.h"

#include <mutex>
#include <string>

#include "common/logger/Logger.h"
#include "infrastructure/camera/protocol/genicam/include/GenApi/GenApi.h"
#include "infrastructure/camera/protocol/gentl/GenTLImpl.h"

using namespace camera_service::infrastructure::gentl;
using namespace GenTL;

namespace {
    std::mutex g_mutex;
    std::string g_last_error;
    bool g_initialized = false;

    void setLastError(const GC_ERROR error, const std::string& message) {
        g_last_error = message;
        LOG_ERROR("GenTL Error {}: {}", static_cast<int>(error), message);
    }

    // Validate handle type casts
    template <typename T>
    T* validateHandle(void* handle) {
        if (handle == nullptr) {
            setLastError(GC_ERR_INVALID_HANDLE, "Null handle");
            return nullptr;
        }
        return static_cast<T*>(handle);
    }
}

//-----------------------------------------------------------------------------
// System Module Functions
//-----------------------------------------------------------------------------
extern "C" {
GC_ERROR GCGetInfo(const TL_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size) {
    // Global library info - no handle needed
    SystemModule temp;
    return temp.getInfo(cmd, type, buffer, size);
}

GC_ERROR GCGetLastError(GC_ERROR* error, char* err_text, size_t* size) {
    std::scoped_lock lock(g_mutex);

    if (error) {
        *error = GC_ERR_ERROR;
    }

    if (size == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    const size_t required = g_last_error.length() + 1;
    if (err_text == nullptr) {
        *size = required;
        return GC_ERR_SUCCESS;
    }

    if (*size < required) {
        *size = required;
        return GC_ERR_BUFFER_TOO_SMALL;
    }

    std::memcpy(err_text, g_last_error.c_str(), required);
    *size = required;
    return GC_ERR_SUCCESS;
}

GC_ERROR GCInitLib() {
    std::scoped_lock lock(g_mutex);
    if (g_initialized) {
        return GC_ERR_SUCCESS;
    }

    LOG_DEBUG("Initializing GenTL library");
    g_initialized = true;
    return GC_ERR_SUCCESS;
}

GC_ERROR GCCloseLib() {
    std::scoped_lock lock(g_mutex);
    if (!g_initialized) {
        return GC_ERR_NOT_INITIALIZED;
    }

    LOG_DEBUG("Closing GenTL library");
    g_initialized = false;
    return GC_ERR_SUCCESS;
}

GC_ERROR GCReadPort(const PORT_HANDLE port, uint64_t address, void* buffer, size_t* size) {
    if (port == nullptr || buffer == nullptr || size == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    try {
        auto* iport = static_cast<GENAPI_NAMESPACE::IPort*>(port);
        iport->Read(buffer, static_cast<int64_t>(address), static_cast<int64_t>(*size));
        return GC_ERR_SUCCESS;
    } catch (const std::exception& e) {
        setLastError(GC_ERR_IO, e.what());
        return GC_ERR_IO;
    }
}

GC_ERROR GCWritePort(PORT_HANDLE port, uint64_t address, const void* buffer, size_t* size) {
    if (port == nullptr || buffer == nullptr || size == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    try {
        auto* iport = static_cast<GENAPI_NAMESPACE::IPort*>(port);
        iport->Write(buffer, static_cast<int64_t>(address), static_cast<int64_t>(*size));
        return GC_ERR_SUCCESS;
    } catch (const std::exception& e) {
        setLastError(GC_ERR_IO, e.what());
        return GC_ERR_IO;
    }
}

GC_ERROR GCGetPortURL(PORT_HANDLE port, char* url, size_t* size) {
    (void)port;

    // Return local XML file path
    const std::string xmlPath = "local:///TMX5x.xml;0x0;0x10000";

    if (size == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    const size_t required = xmlPath.length() + 1;
    if (url == nullptr) {
        *size = required;
        return GC_ERR_SUCCESS;
    }

    if (*size < required) {
        *size = required;
        return GC_ERR_BUFFER_TOO_SMALL;
    }

    std::memcpy(url, xmlPath.c_str(), required);
    *size = required;
    return GC_ERR_SUCCESS;
}

GC_ERROR GCGetPortInfo(PORT_HANDLE port, PORT_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size) {
    (void)port;
    (void)cmd;
    (void)type;
    (void)buffer;
    (void)size;
    return GC_ERR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
// Interface Module Functions
//-----------------------------------------------------------------------------
GC_ERROR TLOpen(TL_HANDLE* tlHandle) {
    if (tlHandle == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    std::scoped_lock lock(g_mutex);

    if (!g_initialized) {
        g_initialized = true;
    }

    try {
        auto* impl = new TLHandleImpl();
        impl->system = std::make_unique<SystemModule>();
        *tlHandle = impl;
        LOG_DEBUG("TLOpen succeeded");
        return GC_ERR_SUCCESS;
    } catch (const std::exception& e) {
        setLastError(GC_ERR_ERROR, e.what());
        return GC_ERR_ERROR;
    }
}

GC_ERROR TLClose(TL_HANDLE tlHandle) {
    auto* impl = validateHandle<TLHandleImpl>(tlHandle);
    if (impl == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    delete impl;
    LOG_DEBUG("TLClose succeeded");
    return GC_ERR_SUCCESS;
}

GC_ERROR TLGetInfo(TL_HANDLE tlHandle, TL_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size) {
    auto* impl = validateHandle<TLHandleImpl>(tlHandle);
    if (impl == nullptr || impl->system == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->system->getInfo(cmd, type, buffer, size);
}

GC_ERROR TLGetNumInterfaces(TL_HANDLE tlHandle, uint32_t* numIfaces) {
    auto* impl = validateHandle<TLHandleImpl>(tlHandle);
    if (impl == nullptr || impl->system == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->system->getNumInterfaces(numIfaces);
}

GC_ERROR TLGetInterfaceID(TL_HANDLE tlHandle, uint32_t index, char* ifaceID, size_t* size) {
    auto* impl = validateHandle<TLHandleImpl>(tlHandle);
    if (impl == nullptr || impl->system == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->system->getInterfaceID(index, ifaceID, size);
}

GC_ERROR TLGetInterfaceInfo(TL_HANDLE tlHandle, const char* ifaceID, INTERFACE_INFO_CMD cmd, INFO_DATATYPE* type,
                            void* buffer, size_t* size) {
    auto* impl = validateHandle<TLHandleImpl>(tlHandle);
    if (impl == nullptr || impl->system == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->system->getInterfaceInfo(ifaceID, cmd, type, buffer, size);
}

GC_ERROR TLOpenInterface(TL_HANDLE tlHandle, const char* ifaceID, IF_HANDLE* ifHandle) {
    auto* impl = validateHandle<TLHandleImpl>(tlHandle);
    if (impl == nullptr || impl->system == nullptr || ifHandle == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    try {
        auto* ifImpl = new IFHandleImpl();
        ifImpl->parent = tlHandle;

        GC_ERROR err = impl->system->openInterface(ifaceID, ifImpl->interface);
        if (err != GC_ERR_SUCCESS) {
            delete ifImpl;
            return err;
        }

        *ifHandle = ifImpl;
        return GC_ERR_SUCCESS;
    } catch (const std::exception& e) {
        setLastError(GC_ERR_ERROR, e.what());
        return GC_ERR_ERROR;
    }
}

GC_ERROR TLUpdateInterfaceList(TL_HANDLE tlHandle, bool8_t* changed, uint64_t timeout) {
    auto* impl = validateHandle<TLHandleImpl>(tlHandle);
    if (impl == nullptr || impl->system == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->system->updateInterfaceList(changed, timeout);
}

GC_ERROR IFClose(IF_HANDLE ifHandle) {
    auto* impl = validateHandle<IFHandleImpl>(ifHandle);
    if (impl == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    delete impl;
    return GC_ERR_SUCCESS;
}

GC_ERROR IFGetInfo(IF_HANDLE ifHandle, INTERFACE_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size) {
    auto* impl = validateHandle<IFHandleImpl>(ifHandle);
    if (impl == nullptr || impl->interface == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->interface->getInfo(cmd, type, buffer, size);
}

GC_ERROR IFGetNumDevices(IF_HANDLE ifHandle, uint32_t* numDevices) {
    auto* impl = validateHandle<IFHandleImpl>(ifHandle);
    if (impl == nullptr || impl->interface == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->interface->getNumDevices(numDevices);
}

GC_ERROR IFGetDeviceID(IF_HANDLE ifHandle, uint32_t index, char* devID, size_t* size) {
    auto* impl = validateHandle<IFHandleImpl>(ifHandle);
    if (impl == nullptr || impl->interface == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->interface->getDeviceID(index, devID, size);
}

GC_ERROR IFUpdateDeviceList(IF_HANDLE ifHandle, bool8_t* changed, uint64_t timeout) {
    auto* impl = validateHandle<IFHandleImpl>(ifHandle);
    if (impl == nullptr || impl->interface == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->interface->updateDeviceList(changed, timeout);
}

GC_ERROR IFGetDeviceInfo(IF_HANDLE ifHandle, const char* devID, DEVICE_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer,
                         size_t* size) {
    auto* impl = validateHandle<IFHandleImpl>(ifHandle);
    if (impl == nullptr || impl->interface == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->interface->getDeviceInfo(devID, cmd, type, buffer, size);
}

GC_ERROR IFOpenDevice(IF_HANDLE ifHandle, const char* devID, DEVICE_ACCESS_STATUS accessMode, DEV_HANDLE* devHandle) {
    auto* impl = validateHandle<IFHandleImpl>(ifHandle);
    if (impl == nullptr || impl->interface == nullptr || devHandle == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    try {
        auto* devImpl = new DevHandleImpl();
        devImpl->parent = ifHandle;

        GC_ERROR err = impl->interface->openDevice(devID, accessMode, devImpl->device);
        if (err != GC_ERR_SUCCESS) {
            delete devImpl;
            return err;
        }

        *devHandle = devImpl;
        return GC_ERR_SUCCESS;
    } catch (const std::exception& e) {
        setLastError(GC_ERR_ERROR, e.what());
        return GC_ERR_ERROR;
    }
}

//-----------------------------------------------------------------------------
// Device Module Functions
//-----------------------------------------------------------------------------
GC_ERROR DevClose(DEV_HANDLE devHandle) {
    auto* impl = validateHandle<DevHandleImpl>(devHandle);
    if (impl == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    delete impl;
    return GC_ERR_SUCCESS;
}

GC_ERROR DevGetInfo(DEV_HANDLE devHandle, DEVICE_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size) {
    auto* impl = validateHandle<DevHandleImpl>(devHandle);
    if (impl == nullptr || impl->device == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->device->getInfo(cmd, type, buffer, size);
}

GC_ERROR DevGetPort(DEV_HANDLE devHandle, PORT_HANDLE* portHandle) {
    auto* impl = validateHandle<DevHandleImpl>(devHandle);
    if (impl == nullptr || impl->device == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->device->getPort(portHandle);
}

GC_ERROR DevGetNumDataStreams(DEV_HANDLE devHandle, uint32_t* numStreams) {
    auto* impl = validateHandle<DevHandleImpl>(devHandle);
    if (impl == nullptr || impl->device == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->device->getNumDataStreams(numStreams);
}

GC_ERROR DevGetDataStreamID(DEV_HANDLE devHandle, uint32_t index, char* streamID, size_t* size) {
    auto* impl = validateHandle<DevHandleImpl>(devHandle);
    if (impl == nullptr || impl->device == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->device->getDataStreamID(index, streamID, size);
}

GC_ERROR DevOpenDataStream(DEV_HANDLE devHandle, const char* streamID, DS_HANDLE* streamHandle) {
    auto* impl = validateHandle<DevHandleImpl>(devHandle);
    if (impl == nullptr || impl->device == nullptr || streamHandle == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    try {
        auto* dsImpl = new DSHandleImpl();
        dsImpl->parent = devHandle;

        GC_ERROR err = impl->device->openDataStream(streamID, dsImpl->stream);
        if (err != GC_ERR_SUCCESS) {
            delete dsImpl;
            return err;
        }

        *streamHandle = dsImpl;
        return GC_ERR_SUCCESS;
    } catch (const std::exception& e) {
        setLastError(GC_ERR_ERROR, e.what());
        return GC_ERR_ERROR;
    }
}

//-----------------------------------------------------------------------------
// Data Stream Module Functions
//-----------------------------------------------------------------------------
GC_ERROR DSClose(DS_HANDLE streamHandle) {
    auto* impl = validateHandle<DSHandleImpl>(streamHandle);
    if (impl == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    delete impl;
    return GC_ERR_SUCCESS;
}

GC_ERROR DSGetInfo(DS_HANDLE streamHandle, STREAM_INFO_CMD cmd, INFO_DATATYPE* type, void* buffer, size_t* size) {
    auto* impl = validateHandle<DSHandleImpl>(streamHandle);
    if (impl == nullptr || impl->stream == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->stream->getInfo(cmd, type, buffer, size);
}

GC_ERROR DSAnnounceBuffer(DS_HANDLE streamHandle, void* buffer, size_t size, void* privateData,
                          BUFFER_HANDLE* bufferHandle) {
    auto* impl = validateHandle<DSHandleImpl>(streamHandle);
    if (impl == nullptr || impl->stream == nullptr || bufferHandle == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    try {
        auto* bufImpl = new BufferHandleImpl();
        bufImpl->parent = streamHandle;

        GC_ERROR err = impl->stream->announceBuffer(buffer, size, privateData, bufImpl->buffer);
        if (err != GC_ERR_SUCCESS) {
            delete bufImpl;
            return err;
        }

        *bufferHandle = bufImpl;
        return GC_ERR_SUCCESS;
    } catch (const std::exception& e) {
        setLastError(GC_ERR_ERROR, e.what());
        return GC_ERR_ERROR;
    }
}

GC_ERROR DSAllocAndAnnounceBuffer(DS_HANDLE streamHandle, size_t size, void* privateData, BUFFER_HANDLE* bufferHandle) {
    auto* impl = validateHandle<DSHandleImpl>(streamHandle);
    if (impl == nullptr || impl->stream == nullptr || bufferHandle == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    try {
        auto* bufImpl = new BufferHandleImpl();
        bufImpl->parent = streamHandle;

        GC_ERROR err = impl->stream->allocAndAnnounceBuffer(size, privateData, bufImpl->buffer);
        if (err != GC_ERR_SUCCESS) {
            delete bufImpl;
            return err;
        }

        *bufferHandle = bufImpl;
        return GC_ERR_SUCCESS;
    } catch (const std::exception& e) {
        setLastError(GC_ERR_ERROR, e.what());
        return GC_ERR_ERROR;
    }
}

GC_ERROR DSFlushQueue(DS_HANDLE streamHandle, ACQ_QUEUE_TYPE operation) {
    auto* impl = validateHandle<DSHandleImpl>(streamHandle);
    if (impl == nullptr || impl->stream == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->stream->flushQueue(operation);
}

GC_ERROR DSStartAcquisition(DS_HANDLE streamHandle, ACQ_START_FLAGS flags, uint64_t numFrames) {
    auto* impl = validateHandle<DSHandleImpl>(streamHandle);
    if (impl == nullptr || impl->stream == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->stream->startAcquisition(flags, numFrames);
}

GC_ERROR DSStopAcquisition(DS_HANDLE streamHandle, ACQ_STOP_FLAGS flags) {
    auto* impl = validateHandle<DSHandleImpl>(streamHandle);
    if (impl == nullptr || impl->stream == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->stream->stopAcquisition(flags);
}

GC_ERROR DSGetBufferID(DS_HANDLE streamHandle, uint32_t index, BUFFER_HANDLE* bufferHandle) {
    auto* impl = validateHandle<DSHandleImpl>(streamHandle);
    if (impl == nullptr || impl->stream == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->stream->getBufferID(index, bufferHandle);
}

GC_ERROR DSRevokeBuffer(DS_HANDLE streamHandle, BUFFER_HANDLE bufferHandle, void** buffer, void** privateData) {
    auto* impl = validateHandle<DSHandleImpl>(streamHandle);
    if (impl == nullptr || impl->stream == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    auto* bufImpl = validateHandle<BufferHandleImpl>(bufferHandle);
    if (bufImpl == nullptr || bufImpl->buffer == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    GC_ERROR err = impl->stream->revokeBuffer(bufImpl->buffer.get(), buffer, privateData);
    if (err == GC_ERR_SUCCESS) {
        delete bufImpl;
    }

    return err;
}

GC_ERROR DSQueueBuffer(DS_HANDLE streamHandle, BUFFER_HANDLE bufferHandle) {
    auto* impl = validateHandle<DSHandleImpl>(streamHandle);
    if (impl == nullptr || impl->stream == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    auto* bufImpl = validateHandle<BufferHandleImpl>(bufferHandle);
    if (bufImpl == nullptr || bufImpl->buffer == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->stream->queueBuffer(bufImpl->buffer.get());
}

GC_ERROR DSGetBufferInfo(DS_HANDLE streamHandle, BUFFER_HANDLE bufferHandle, BUFFER_INFO_CMD cmd, INFO_DATATYPE* type,
                         void* buffer, size_t* size) {
    auto* impl = validateHandle<DSHandleImpl>(streamHandle);
    if (impl == nullptr || impl->stream == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    auto* bufImpl = validateHandle<BufferHandleImpl>(bufferHandle);
    if (bufImpl == nullptr || bufImpl->buffer == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    return impl->stream->getBufferInfo(bufImpl->buffer.get(), cmd, type, buffer, size);
}

//-----------------------------------------------------------------------------
// Event Module Functions (Stubs)
//-----------------------------------------------------------------------------
GC_ERROR GCRegisterEvent(BUFFER_HANDLE handle, int32_t eventType, EVENT_HANDLE* eventHandle) {
    (void)handle;
    (void)eventType;
    (void)eventHandle;
    return GC_ERR_NOT_IMPLEMENTED;
}

GC_ERROR GCUnregisterEvent(BUFFER_HANDLE handle, int32_t eventType) {
    (void)handle;
    (void)eventType;
    return GC_ERR_NOT_IMPLEMENTED;
}

GC_ERROR EventGetData(EVENT_HANDLE eventHandle, void* buffer, size_t* size, uint64_t timeout) {
    (void)eventHandle;
    (void)buffer;
    (void)size;
    (void)timeout;
    return GC_ERR_NOT_IMPLEMENTED;
}

GC_ERROR EventGetDataInfo(EVENT_HANDLE eventHandle, const void* pInBuffer, size_t iInSize, EVENT_DATA_INFO_CMD cmd,
                          INFO_DATATYPE* type, void* buffer, size_t* size) {
    (void)eventHandle;
    (void)pInBuffer;
    (void)iInSize;
    (void)cmd;
    (void)type;
    (void)buffer;
    (void)size;
    return GC_ERR_NOT_IMPLEMENTED;
}

GC_ERROR EventGetInfo(EVENT_HANDLE eventHandle, int32_t cmd, INFO_DATATYPE* type, void* buffer, size_t* size) {
    (void)eventHandle;
    (void)cmd;
    (void)type;
    (void)buffer;
    (void)size;
    return GC_ERR_NOT_IMPLEMENTED;
}

GC_ERROR EventFlush(EVENT_HANDLE eventHandle) {
    (void)eventHandle;
    return GC_ERR_NOT_IMPLEMENTED;
}

GC_ERROR EventKill(EVENT_HANDLE eventHandle) {
    (void)eventHandle;
    return GC_ERR_NOT_IMPLEMENTED;
}
}