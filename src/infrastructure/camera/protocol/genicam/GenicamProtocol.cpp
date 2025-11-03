#include "GenicamProtocol.h"

namespace camera_service::infrastructure {
    GenicamProtocol::GenicamProtocol(std::unique_ptr<GENAPI_NAMESPACE::IPort> transport)
        : transport_(std::move(transport)) {
        if (!transport_) {
            throw std::invalid_argument("Transport port cannot be null");
        }
    }

    GenicamProtocol::~GenicamProtocol() {
        if (close().isError()) {
            LOG_ERROR("Failed to close GenICam protocol");
        }
    }

    Result<void> GenicamProtocol::open() {
        try {
            node_map_._LoadXMLFromFile(
                "/home/shalex/dev/projects/camera-service/src/infrastructure/camera/protocol/genicam/TMX5x.xml");
            node_map_._Connect(transport_.get());
        } catch (const std::exception& e) {
            return Result<void>::error(e.what());
        }

        if (!setEnum("ConnectionConfig", "CXP3_X1")) {
            return Result<void>::error("Failed to set ConnectionConfig");
        }

        return Result<void>::success();
    }

    Result<void> GenicamProtocol::close() {
        return Result<void>::success();
    }

    Result<void> GenicamProtocol::setExposureTime(const double time_us) const {
        if (!setFloat("ExposureTime", time_us)) {
            return Result<void>::error("Failed to set exposure time");
        }

        return Result<void>::success();
    }

    bool GenicamProtocol::setFloat(const std::string& feature, const double value) const {
        try {
            const GENAPI_NAMESPACE::CFloatPtr node = node_map_._GetNode(feature.c_str());
            if (!GENAPI_NAMESPACE::IsWritable(node)) {
                return false;
            }
            node->SetValue(value);
            return true;
        } catch (...) {
            return false;
        }
    }

    bool GenicamProtocol::setInteger(const std::string& feature, const int64_t value) const {
        try {
            const GENAPI_NAMESPACE::CIntegerPtr node = node_map_._GetNode(feature.c_str());
            if (!GENAPI_NAMESPACE::IsWritable(node)) {
                return false;
            }
            node->SetValue(value);
            return true;
        } catch (...) {
            return false;
        }
    }

    bool GenicamProtocol::setBoolean(const std::string& feature, const bool value) const {
        try {
            const GENAPI_NAMESPACE::CBooleanPtr node = node_map_._GetNode(feature.c_str());
            if (!GENAPI_NAMESPACE::IsWritable(node)) {
                return false;
            }
            node->SetValue(value);
            return true;
        } catch (...) {
            return false;
        }
    }

    bool GenicamProtocol::setEnum(const std::string& feature, const std::string& value) const {
        try {
            const GENAPI_NAMESPACE::CEnumerationPtr node = node_map_._GetNode(feature.c_str());
            if (!GENAPI_NAMESPACE::IsWritable(node)) {
                return false;
            }
            const GENAPI_NAMESPACE::CEnumEntryPtr entry = node->GetEntryByName(value.c_str());
            if (!GENAPI_NAMESPACE::IsAvailable(entry)) {
                return false;
            }
            node->SetIntValue(entry->GetValue());
            return true;
        } catch (...) {
            return false;
        }
    }

    bool GenicamProtocol::executeCommand(const std::string& feature) const {
        try {
            const GENAPI_NAMESPACE::CCommandPtr node = node_map_._GetNode(feature.c_str());
            if (!GENAPI_NAMESPACE::IsWritable(node)) {
                return false;
            }
            node->Execute();
            return true;
        } catch (...) {
            return false;
        }
    }
}