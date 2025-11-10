#include "AdimecGenTLCamera.h"

#include "common/logger/Logger.h"
#include "infrastructure/camera/protocol/itl/ItlProtocol.h"
#include <cstring>

namespace camera_service::infrastructure {
    AdimecGenTLCamera::AdimecGenTLCamera(std::string producer_path, std::unique_ptr<ItlProtocol> lens_protocol)
        : producer_path_(std::move(producer_path)), lens_protocol_(std::move(lens_protocol)) {}

    AdimecGenTLCamera::~AdimecGenTLCamera() {
        if (cleanupGenTL().isError()) {
            LOG_ERROR("Failed to cleanup GenTL resources");
        }
    }

    Result<void> AdimecGenTLCamera::open() {
        // Initialize GenTL Producer
        if (auto init_result = initializeGenTL(); init_result.isError()) {
            return init_result;
        }

        // Discover and open device
        if (auto device_result = discoverAndOpenDevice(); device_result.isError()) {
            cleanupGenTL();
            return device_result;
        }

        // Open lens protocol if provided
        if (lens_protocol_) {
            if (const auto lens_result = lens_protocol_->open(); lens_result.isError()) {
                cleanupGenTL();
                return Result<void>::error("Failed to open lens protocol: " + lens_result.error());
            }
        }

        LOG_DEBUG("GenTL Camera opened successfully");
        return Result<void>::success();
    }

    Result<void> AdimecGenTLCamera::close() {
        if (stream_handle_ && gentl_) {
            gentl_->DSStopAcquisition(stream_handle_, GenTL::ACQ_STOP_FLAGS_KILL);
        }

        if (lens_protocol_) {
            if (const auto lens_result = lens_protocol_->close(); lens_result.isError()) {
                LOG_WARN("Failed to close lens protocol: {}", lens_result.error());
            }
        }

        return cleanupGenTL();
    }

    Result<types::info> AdimecGenTLCamera::getInfo() const {
        if (!dev_handle_) {
            return Result<types::info>::error("Device not opened");
        }

        std::string info;
        char buffer[512];
        size_t size;
        GenTL::INFO_DATATYPE type;

        // Get vendor name
        size = sizeof(buffer);
        if (gentl_->DevGetInfo(dev_handle_, GenTL::DEVICE_INFO_VENDOR, &type, buffer, &size) == GenTL::GC_ERR_SUCCESS) {
            info += "Vendor: " + std::string(buffer) + ", ";
        }

        // Get model name
        size = sizeof(buffer);
        if (gentl_->DevGetInfo(dev_handle_, GenTL::DEVICE_INFO_MODEL, &type, buffer, &size) == GenTL::GC_ERR_SUCCESS) {
            info += "Model: " + std::string(buffer) + ", ";
        }

        // Get serial number
        size = sizeof(buffer);
        if (gentl_->DevGetInfo(dev_handle_, GenTL::DEVICE_INFO_SERIAL_NUMBER, &type, buffer, &size) ==
            GenTL::GC_ERR_SUCCESS) {
            info += "Serial: " + std::string(buffer) + ", ";
        }

        // Get firmware version
        size = sizeof(buffer);
        if (gentl_->DevGetInfo(dev_handle_, GenTL::DEVICE_INFO_VERSION, &type, buffer, &size) ==
            GenTL::GC_ERR_SUCCESS) {
            info += "Firmware: " + std::string(buffer);
        }

        if (info.empty()) {
            return Result<types::info>::error("Failed to retrieve camera information");
        }

        return Result<types::info>::success(info);
    }

    Result<void> AdimecGenTLCamera::startAcquisition() {
        if (!stream_handle_ || !gentl_) {
            return Result<void>::error("Stream not opened");
        }

        auto err = gentl_->DSStartAcquisition(stream_handle_, GenTL::ACQ_START_FLAGS_DEFAULT, GENTL_INFINITE);
        if (err != GenTL::GC_ERR_SUCCESS) {
            return Result<void>::error("Failed to start acquisition: error " + std::to_string(err));
        }

        LOG_INFO("Acquisition started");
        return Result<void>::success();
    }

    Result<void> AdimecGenTLCamera::stopAcquisition() {
        if (!stream_handle_ || !gentl_) {
            return Result<void>::error("Stream not opened");
        }

        auto err = gentl_->DSStopAcquisition(stream_handle_, GenTL::ACQ_STOP_FLAGS_DEFAULT);
        if (err != GenTL::GC_ERR_SUCCESS) {
            return Result<void>::error("Failed to stop acquisition: error " + std::to_string(err));
        }

        LOG_INFO("Acquisition stopped");
        return Result<void>::success();
    }

    Result<void> AdimecGenTLCamera::initializeGenTL() {
        // Load GenTL Producer library
        auto loader_result = GenTLLoader::load(producer_path_);
        if (loader_result.isError()) {
            return Result<void>::error("Failed to load GenTL Producer: " + loader_result.error());
        }
        gentl_ = std::move(loader_result).value();

        // Initialize GenTL library
        auto err = gentl_->GCInitLib();
        if (err != GenTL::GC_ERR_SUCCESS) {
            return Result<void>::error("Failed to initialize GenTL library");
        }

        // Open Transport Layer
        err = gentl_->TLOpen(&tl_handle_);
        if (err != GenTL::GC_ERR_SUCCESS) {
            return Result<void>::error("Failed to open Transport Layer");
        }

        LOG_DEBUG("GenTL Transport Layer opened");
        return Result<void>::success();
    }

    Result<void> AdimecGenTLCamera::discoverAndOpenDevice() {
        // Update interface list
        uint32_t num_interfaces = 0;
        auto err = gentl_->TLGetNumInterfaces(tl_handle_, &num_interfaces);
        if (err != GenTL::GC_ERR_SUCCESS || num_interfaces == 0) {
            return Result<void>::error("No GenTL interfaces found");
        }

        LOG_DEBUG("Found {} GenTL interface(s)", num_interfaces);

        // Get first interface ID
        char if_id[256];
        size_t size = sizeof(if_id);
        err = gentl_->TLGetInterfaceID(tl_handle_, 0, if_id, &size);
        if (err != GenTL::GC_ERR_SUCCESS) {
            return Result<void>::error("Failed to get interface ID");
        }

        // Open interface
        err = gentl_->TLOpenInterface(tl_handle_, if_id, &if_handle_);
        if (err != GenTL::GC_ERR_SUCCESS) {
            return Result<void>::error("Failed to open interface: " + std::string(if_id));
        }

        LOG_DEBUG("Opened interface: {}", if_id);

        // Update device list
        uint32_t num_devices = 0;
        err = gentl_->IFGetNumDevices(if_handle_, &num_devices);
        if (err != GenTL::GC_ERR_SUCCESS || num_devices == 0) {
            return Result<void>::error("No devices found on interface");
        }

        LOG_DEBUG("Found {} device(s) on interface", num_devices);

        // Get first device ID
        char dev_id[256];
        size = sizeof(dev_id);
        err = gentl_->IFGetDeviceID(if_handle_, 0, dev_id, &size);
        if (err != GenTL::GC_ERR_SUCCESS) {
            return Result<void>::error("Failed to get device ID");
        }

        // Open device with exclusive access
        err = gentl_->IFOpenDevice(if_handle_, dev_id, GenTL::DEVICE_ACCESS_EXCLUSIVE, &dev_handle_);
        if (err != GenTL::GC_ERR_SUCCESS) {
            return Result<void>::error("Failed to open device: " + std::string(dev_id));
        }

        LOG_INFO("Opened device: {}", dev_id);

        // Get device port for GenApi access
        err = gentl_->DevGetPort(dev_handle_, &device_port_);
        if (err != GenTL::GC_ERR_SUCCESS) {
            return Result<void>::error("Failed to get device port");
        }

        // Get first data stream
        uint32_t num_streams = 0;
        err = gentl_->DevGetNumDataStreams(dev_handle_, &num_streams);
        if (err != GenTL::GC_ERR_SUCCESS || num_streams == 0) {
            return Result<void>::error("No data streams available");
        }

        char stream_id[256];
        size = sizeof(stream_id);
        err = gentl_->DevGetDataStreamID(dev_handle_, 0, stream_id, &size);
        if (err != GenTL::GC_ERR_SUCCESS) {
            return Result<void>::error("Failed to get stream ID");
        }

        // Open data stream
        err = gentl_->DevOpenDataStream(dev_handle_, stream_id, &stream_handle_);
        if (err != GenTL::GC_ERR_SUCCESS) {
            return Result<void>::error("Failed to open data stream");
        }

        LOG_DEBUG("Opened data stream: {}", stream_id);
        return Result<void>::success();
    }

    Result<void> AdimecGenTLCamera::cleanupGenTL() {
        if (!gentl_) {
            return Result<void>::success();
        }

        bool had_errors = false;

        // Close data stream
        if (stream_handle_) {
            if (gentl_->DSClose(stream_handle_) != GenTL::GC_ERR_SUCCESS) {
                LOG_ERROR("Failed to close data stream");
                had_errors = true;
            }
            stream_handle_ = nullptr;
        }

        // Close device
        if (dev_handle_) {
            if (gentl_->DevClose(dev_handle_) != GenTL::GC_ERR_SUCCESS) {
                LOG_ERROR("Failed to close device");
                had_errors = true;
            }
            dev_handle_ = nullptr;
            device_port_ = nullptr;
        }

        // Close interface
        if (if_handle_) {
            if (gentl_->IFClose(if_handle_) != GenTL::GC_ERR_SUCCESS) {
                LOG_ERROR("Failed to close interface");
                had_errors = true;
            }
            if_handle_ = nullptr;
        }

        // Close Transport Layer
        if (tl_handle_) {
            if (gentl_->TLClose(tl_handle_) != GenTL::GC_ERR_SUCCESS) {
                LOG_ERROR("Failed to close Transport Layer");
                had_errors = true;
            }
            tl_handle_ = nullptr;
        }

        // GCCloseLib is called automatically in GenTLLoader destructor
        gentl_.reset();

        if (had_errors) {
            return Result<void>::error("Errors occurred during GenTL cleanup");
        }

        LOG_DEBUG("GenTL resources cleaned up");
        return Result<void>::success();
    }
}