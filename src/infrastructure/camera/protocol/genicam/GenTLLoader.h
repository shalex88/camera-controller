#pragma once

#include "infrastructure/camera/protocol/gentl/GenTL.h"
#include "common/types/Result.h"
#include <string>
#include <memory>

namespace camera_service::infrastructure {

/**
 * @brief Dynamic loader for GenTL Producer libraries (.cti files)
 *
 * This class handles runtime loading of GenTL Producer shared libraries
 * and provides access to GenTL API functions through function pointers.
 */
class GenTLLoader {
public:
    // GenTL API function pointer types
    using GCInitLib_t = GenTL::GC_ERROR (*)();
    using GCCloseLib_t = GenTL::GC_ERROR (*)();
    using GCGetInfo_t = GenTL::GC_ERROR (*)(GenTL::TL_INFO_CMD, GenTL::INFO_DATATYPE*, void*, size_t*);
    using GCGetLastError_t = GenTL::GC_ERROR (*)(GenTL::GC_ERROR*, char*, size_t*);
    using GCReadPort_t = GenTL::GC_ERROR (*)(GenTL::PORT_HANDLE, uint64_t, void*, size_t*);
    using GCWritePort_t = GenTL::GC_ERROR (*)(GenTL::PORT_HANDLE, uint64_t, const void*, size_t*);
    using GCGetPortURL_t = GenTL::GC_ERROR (*)(GenTL::PORT_HANDLE, char*, size_t*);
    using GCGetPortInfo_t = GenTL::GC_ERROR (*)(GenTL::PORT_HANDLE, GenTL::PORT_INFO_CMD, GenTL::INFO_DATATYPE*, void*, size_t*);

    using TLOpen_t = GenTL::GC_ERROR (*)(GenTL::TL_HANDLE*);
    using TLClose_t = GenTL::GC_ERROR (*)(GenTL::TL_HANDLE);
    using TLGetInfo_t = GenTL::GC_ERROR (*)(GenTL::TL_HANDLE, GenTL::TL_INFO_CMD, GenTL::INFO_DATATYPE*, void*, size_t*);
    using TLGetNumInterfaces_t = GenTL::GC_ERROR (*)(GenTL::TL_HANDLE, uint32_t*);
    using TLGetInterfaceID_t = GenTL::GC_ERROR (*)(GenTL::TL_HANDLE, uint32_t, char*, size_t*);
    using TLGetInterfaceInfo_t = GenTL::GC_ERROR (*)(GenTL::TL_HANDLE, const char*, GenTL::INTERFACE_INFO_CMD, GenTL::INFO_DATATYPE*, void*, size_t*);
    using TLOpenInterface_t = GenTL::GC_ERROR (*)(GenTL::TL_HANDLE, const char*, GenTL::IF_HANDLE*);
    using TLUpdateInterfaceList_t = GenTL::GC_ERROR (*)(GenTL::TL_HANDLE, bool8_t*, uint64_t);

    using IFClose_t = GenTL::GC_ERROR (*)(GenTL::IF_HANDLE);
    using IFGetInfo_t = GenTL::GC_ERROR (*)(GenTL::IF_HANDLE, GenTL::INTERFACE_INFO_CMD, GenTL::INFO_DATATYPE*, void*, size_t*);
    using IFGetNumDevices_t = GenTL::GC_ERROR (*)(GenTL::IF_HANDLE, uint32_t*);
    using IFGetDeviceID_t = GenTL::GC_ERROR (*)(GenTL::IF_HANDLE, uint32_t, char*, size_t*);
    using IFUpdateDeviceList_t = GenTL::GC_ERROR (*)(GenTL::IF_HANDLE, bool8_t*, uint64_t);
    using IFGetDeviceInfo_t = GenTL::GC_ERROR (*)(GenTL::IF_HANDLE, const char*, GenTL::DEVICE_INFO_CMD, GenTL::INFO_DATATYPE*, void*, size_t*);
    using IFOpenDevice_t = GenTL::GC_ERROR (*)(GenTL::IF_HANDLE, const char*, GenTL::DEVICE_ACCESS_FLAGS, GenTL::DEV_HANDLE*);

    using DevGetPort_t = GenTL::GC_ERROR (*)(GenTL::DEV_HANDLE, GenTL::PORT_HANDLE*);
    using DevGetNumDataStreams_t = GenTL::GC_ERROR (*)(GenTL::DEV_HANDLE, uint32_t*);
    using DevGetDataStreamID_t = GenTL::GC_ERROR (*)(GenTL::DEV_HANDLE, uint32_t, char*, size_t*);
    using DevOpenDataStream_t = GenTL::GC_ERROR (*)(GenTL::DEV_HANDLE, const char*, GenTL::DS_HANDLE*);
    using DevGetInfo_t = GenTL::GC_ERROR (*)(GenTL::DEV_HANDLE, GenTL::DEVICE_INFO_CMD, GenTL::INFO_DATATYPE*, void*, size_t*);
    using DevClose_t = GenTL::GC_ERROR (*)(GenTL::DEV_HANDLE);

    using DSAnnounceBuffer_t = GenTL::GC_ERROR (*)(GenTL::DS_HANDLE, void*, size_t, void*, GenTL::BUFFER_HANDLE*);
    using DSAllocAndAnnounceBuffer_t = GenTL::GC_ERROR (*)(GenTL::DS_HANDLE, size_t, void*, GenTL::BUFFER_HANDLE*);
    using DSFlushQueue_t = GenTL::GC_ERROR (*)(GenTL::DS_HANDLE, GenTL::ACQ_QUEUE_TYPE);
    using DSStartAcquisition_t = GenTL::GC_ERROR (*)(GenTL::DS_HANDLE, GenTL::ACQ_START_FLAGS, uint64_t);
    using DSStopAcquisition_t = GenTL::GC_ERROR (*)(GenTL::DS_HANDLE, GenTL::ACQ_STOP_FLAGS);
    using DSGetInfo_t = GenTL::GC_ERROR (*)(GenTL::DS_HANDLE, GenTL::STREAM_INFO_CMD, GenTL::INFO_DATATYPE*, void*, size_t*);
    using DSGetBufferID_t = GenTL::GC_ERROR (*)(GenTL::DS_HANDLE, uint32_t, GenTL::BUFFER_HANDLE*);
    using DSClose_t = GenTL::GC_ERROR (*)(GenTL::DS_HANDLE);
    using DSRevokeBuffer_t = GenTL::GC_ERROR (*)(GenTL::DS_HANDLE, GenTL::BUFFER_HANDLE, void**, void**);
    using DSQueueBuffer_t = GenTL::GC_ERROR (*)(GenTL::DS_HANDLE, GenTL::BUFFER_HANDLE);
    using DSGetBufferInfo_t = GenTL::GC_ERROR (*)(GenTL::DS_HANDLE, GenTL::BUFFER_HANDLE, GenTL::BUFFER_INFO_CMD, GenTL::INFO_DATATYPE*, void*, size_t*);

    using GCRegisterEvent_t = GenTL::GC_ERROR (*)(GenTL::EVENTSRC_HANDLE, GenTL::EVENT_TYPE, GenTL::EVENT_HANDLE*);
    using GCUnregisterEvent_t = GenTL::GC_ERROR (*)(GenTL::EVENTSRC_HANDLE, GenTL::EVENT_TYPE);
    using EventGetData_t = GenTL::GC_ERROR (*)(GenTL::EVENT_HANDLE, void*, size_t*, uint64_t);
    using EventGetDataInfo_t = GenTL::GC_ERROR (*)(GenTL::EVENT_HANDLE, const void*, size_t, GenTL::EVENT_DATA_INFO_CMD, GenTL::INFO_DATATYPE*, void*, size_t*);
    using EventGetInfo_t = GenTL::GC_ERROR (*)(GenTL::EVENT_HANDLE, GenTL::EVENT_INFO_CMD, GenTL::INFO_DATATYPE*, void*, size_t*);
    using EventFlush_t = GenTL::GC_ERROR (*)(GenTL::EVENT_HANDLE);
    using EventKill_t = GenTL::GC_ERROR (*)(GenTL::EVENT_HANDLE);

    // Function pointers for all GenTL API functions
    GCInitLib_t GCInitLib = nullptr;
    GCCloseLib_t GCCloseLib = nullptr;
    GCGetInfo_t GCGetInfo = nullptr;
    GCGetLastError_t GCGetLastError = nullptr;
    GCReadPort_t GCReadPort = nullptr;
    GCWritePort_t GCWritePort = nullptr;
    GCGetPortURL_t GCGetPortURL = nullptr;
    GCGetPortInfo_t GCGetPortInfo = nullptr;

    TLOpen_t TLOpen = nullptr;
    TLClose_t TLClose = nullptr;
    TLGetInfo_t TLGetInfo = nullptr;
    TLGetNumInterfaces_t TLGetNumInterfaces = nullptr;
    TLGetInterfaceID_t TLGetInterfaceID = nullptr;
    TLGetInterfaceInfo_t TLGetInterfaceInfo = nullptr;
    TLOpenInterface_t TLOpenInterface = nullptr;
    TLUpdateInterfaceList_t TLUpdateInterfaceList = nullptr;

    IFClose_t IFClose = nullptr;
    IFGetInfo_t IFGetInfo = nullptr;
    IFGetNumDevices_t IFGetNumDevices = nullptr;
    IFGetDeviceID_t IFGetDeviceID = nullptr;
    IFUpdateDeviceList_t IFUpdateDeviceList = nullptr;
    IFGetDeviceInfo_t IFGetDeviceInfo = nullptr;
    IFOpenDevice_t IFOpenDevice = nullptr;

    DevGetPort_t DevGetPort = nullptr;
    DevGetNumDataStreams_t DevGetNumDataStreams = nullptr;
    DevGetDataStreamID_t DevGetDataStreamID = nullptr;
    DevOpenDataStream_t DevOpenDataStream = nullptr;
    DevGetInfo_t DevGetInfo = nullptr;
    DevClose_t DevClose = nullptr;

    DSAnnounceBuffer_t DSAnnounceBuffer = nullptr;
    DSAllocAndAnnounceBuffer_t DSAllocAndAnnounceBuffer = nullptr;
    DSFlushQueue_t DSFlushQueue = nullptr;
    DSStartAcquisition_t DSStartAcquisition = nullptr;
    DSStopAcquisition_t DSStopAcquisition = nullptr;
    DSGetInfo_t DSGetInfo = nullptr;
    DSGetBufferID_t DSGetBufferID = nullptr;
    DSClose_t DSClose = nullptr;
    DSRevokeBuffer_t DSRevokeBuffer = nullptr;
    DSQueueBuffer_t DSQueueBuffer = nullptr;
    DSGetBufferInfo_t DSGetBufferInfo = nullptr;

    GCRegisterEvent_t GCRegisterEvent = nullptr;
    GCUnregisterEvent_t GCUnregisterEvent = nullptr;
    EventGetData_t EventGetData = nullptr;
    EventGetDataInfo_t EventGetDataInfo = nullptr;
    EventGetInfo_t EventGetInfo = nullptr;
    EventFlush_t EventFlush = nullptr;
    EventKill_t EventKill = nullptr;

    /**
     * @brief Load a GenTL Producer library
     * @param library_path Path to the .cti file
     * @return Result containing the loaded GenTLLoader or error message
     */
    static Result<std::unique_ptr<GenTLLoader>> load(const std::string& library_path);

    ~GenTLLoader();

    // Prevent copying
    GenTLLoader(const GenTLLoader&) = delete;
    GenTLLoader& operator=(const GenTLLoader&) = delete;

    // Allow moving
    GenTLLoader(GenTLLoader&&) noexcept = default;
    GenTLLoader& operator=(GenTLLoader&&) noexcept = default;

    /**
     * @brief Check if the library is loaded and all required functions are available
     */
    bool isLoaded() const { return library_handle_ != nullptr; }

    /**
     * @brief Get the path to the loaded library
     */
    const std::string& getLibraryPath() const { return library_path_; }

private:
    GenTLLoader() = default;

    Result<void> loadLibrary(const std::string& path);
    Result<void> loadFunctions();

    template<typename FuncPtr>
    Result<void> loadFunction(FuncPtr& func_ptr, const char* func_name);

    void* library_handle_ = nullptr;
    std::string library_path_;
};

} // namespace camera_service::infrastructure
