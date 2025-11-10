#include "GenTLLoader.h"
#include "common/logger/Logger.h"
#include <dlfcn.h>

namespace camera_service::infrastructure {

Result<std::unique_ptr<GenTLLoader>> GenTLLoader::load(const std::string& library_path) {
    auto loader = std::unique_ptr<GenTLLoader>(new GenTLLoader());
    
    auto load_result = loader->loadLibrary(library_path);
    if (load_result.isError()) {
        return Result<std::unique_ptr<GenTLLoader>>::error(load_result.error());
    }
    
    auto func_result = loader->loadFunctions();
    if (func_result.isError()) {
        return Result<std::unique_ptr<GenTLLoader>>::error(func_result.error());
    }
    
    LOG_INFO("GenTL Producer loaded successfully: {}", library_path);
    return Result<std::unique_ptr<GenTLLoader>>::success(std::move(loader));
}

GenTLLoader::~GenTLLoader() {
    if (library_handle_) {
        // Close library if initialized
        if (GCCloseLib) {
            GCCloseLib();
        }
        dlclose(library_handle_);
        library_handle_ = nullptr;
        LOG_DEBUG("GenTL Producer library unloaded: {}", library_path_);
    }
}

Result<void> GenTLLoader::loadLibrary(const std::string& path) {
    library_handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!library_handle_) {
        const char* error = dlerror();
        return Result<void>::error(std::string("Failed to load GenTL Producer: ") + 
                                   (error ? error : "Unknown error"));
    }
    
    library_path_ = path;
    LOG_DEBUG("GenTL Producer library opened: {}", path);
    return Result<void>::success();
}

Result<void> GenTLLoader::loadFunctions() {
    // Load all GenTL API functions
    #define LOAD_FUNC(name) \
        if (auto result = loadFunction(name, #name); result.isError()) { \
            return result; \
        }
    
    // Library functions
    LOAD_FUNC(GCInitLib);
    LOAD_FUNC(GCCloseLib);
    LOAD_FUNC(GCGetInfo);
    LOAD_FUNC(GCGetLastError);
    LOAD_FUNC(GCReadPort);
    LOAD_FUNC(GCWritePort);
    LOAD_FUNC(GCGetPortURL);
    LOAD_FUNC(GCGetPortInfo);
    
    // Transport Layer functions
    LOAD_FUNC(TLOpen);
    LOAD_FUNC(TLClose);
    LOAD_FUNC(TLGetInfo);
    LOAD_FUNC(TLGetNumInterfaces);
    LOAD_FUNC(TLGetInterfaceID);
    LOAD_FUNC(TLGetInterfaceInfo);
    LOAD_FUNC(TLOpenInterface);
    LOAD_FUNC(TLUpdateInterfaceList);
    
    // Interface functions
    LOAD_FUNC(IFClose);
    LOAD_FUNC(IFGetInfo);
    LOAD_FUNC(IFGetNumDevices);
    LOAD_FUNC(IFGetDeviceID);
    LOAD_FUNC(IFUpdateDeviceList);
    LOAD_FUNC(IFGetDeviceInfo);
    LOAD_FUNC(IFOpenDevice);
    
    // Device functions
    LOAD_FUNC(DevGetPort);
    LOAD_FUNC(DevGetNumDataStreams);
    LOAD_FUNC(DevGetDataStreamID);
    LOAD_FUNC(DevOpenDataStream);
    LOAD_FUNC(DevGetInfo);
    LOAD_FUNC(DevClose);
    
    // Data Stream functions
    LOAD_FUNC(DSAnnounceBuffer);
    LOAD_FUNC(DSAllocAndAnnounceBuffer);
    LOAD_FUNC(DSFlushQueue);
    LOAD_FUNC(DSStartAcquisition);
    LOAD_FUNC(DSStopAcquisition);
    LOAD_FUNC(DSGetInfo);
    LOAD_FUNC(DSGetBufferID);
    LOAD_FUNC(DSClose);
    LOAD_FUNC(DSRevokeBuffer);
    LOAD_FUNC(DSQueueBuffer);
    LOAD_FUNC(DSGetBufferInfo);
    
    // Event functions
    LOAD_FUNC(GCRegisterEvent);
    LOAD_FUNC(GCUnregisterEvent);
    LOAD_FUNC(EventGetData);
    LOAD_FUNC(EventGetDataInfo);
    LOAD_FUNC(EventGetInfo);
    LOAD_FUNC(EventFlush);
    LOAD_FUNC(EventKill);
    
    #undef LOAD_FUNC
    
    LOG_DEBUG("All GenTL functions loaded successfully");
    return Result<void>::success();
}

template<typename FuncPtr>
Result<void> GenTLLoader::loadFunction(FuncPtr& func_ptr, const char* func_name) {
    // Clear any previous errors
    dlerror();
    
    void* symbol = dlsym(library_handle_, func_name);
    const char* error = dlerror();
    
    if (error != nullptr) {
        return Result<void>::error(std::string("Failed to load function '") + 
                                   func_name + "': " + error);
    }
    
    func_ptr = reinterpret_cast<FuncPtr>(symbol);
    return Result<void>::success();
}

} // namespace camera_service::infrastructure
