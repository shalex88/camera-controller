#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "api/ITransport.h"
#include "api/IRequestHandler.h"
#include "core/ICore.h"
#include "data/ICameraHal.h"
#include "data/ICameraHw.h"
#include "data/transport/mmio/IRegisterImpl.h"
#include "common/types/CameraCapabilities.h"

using namespace camera_service;
using namespace testing;

class TransportMock: public api::ITransport {
public:
    MOCK_METHOD(Result<void>, start, (const std::string&), (override));
    MOCK_METHOD(Result<void>, stop, (), (override));
    MOCK_METHOD(Result<void>, runLoop, (), (override));
};

class RequestHandlerMock: public api::IRequestHandler {
public:
    MOCK_METHOD(Result<void>, start, (), (override));
    MOCK_METHOD(Result<void>, stop, (), (override));
    MOCK_METHOD(bool, isRunning, (), (const, override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(Result<void>, enableAutoFocus, (bool), (const, override));
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));
    MOCK_METHOD(Result<void>, goToMinZoom, (), (const, override));
    MOCK_METHOD(Result<void>, goToMaxZoom, (), (const, override));
    MOCK_METHOD(Result<void>, stabilize, (bool), (const, override));
};

class CoreMock: public core::ICore {
public:
    MOCK_METHOD(Result<void>, initialize, (), (override));
    MOCK_METHOD(Result<void>, shutdown, (), (override));

    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, goToMinZoom, (), (const, override));
    MOCK_METHOD(Result<void>, goToMaxZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));
    MOCK_METHOD(Result<void>, enableAutoFocus, (bool), (const, override));
    MOCK_METHOD(Result<void>, stabilize, (bool), (const, override));
};

class MockCameraHal : public data::ICameraHal {
public:
    MOCK_METHOD(Result<void>, connect, (), (override));
    MOCK_METHOD(Result<void>, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));

    // IZoomCapable implementation
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(types::ZoomRange, getZoomLimits, (), (const, override));

    // IFocusCapable implementation
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(types::FocusRange, getFocusLimits, (), (const, override));

    // IAutoFocusCapable implementation
    MOCK_METHOD(Result<void>, enableAutoFocus, (bool), (const, override));

    // IInfoCapable implementation
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));

    // IStabilizationCapable implementation
    MOCK_METHOD(Result<void>, stabilize, (bool), (const, override));
};

class MockCameraHw: public data::ICameraHw,
                     public capabilities::IZoomCapable,
                     public capabilities::IFocusCapable,
                     public capabilities::IAutoFocusCapable,
                     public capabilities::IStabilizeCapable,
                     public capabilities::IInfoCapable {
public:
    MockCameraHw() {
        // Set up default behavior for zoom and focus limits to prevent constructor validation failures
        const types::ZoomRange default_zoom_limits{.min = 0, .max = 1000};
        const types::FocusRange default_focus_limits{.min = 0, .max = 1000};

        ON_CALL(*this, getZoomLimits())
            .WillByDefault(Return(default_zoom_limits));
        ON_CALL(*this, getFocusLimits())
            .WillByDefault(Return(default_focus_limits));
    }

    MOCK_METHOD(Result<void>, connect, (), (override));
    MOCK_METHOD(Result<void>, disconnect, (), (override));

    // IZoomCapable implementation
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(types::ZoomRange, getZoomLimits, (), (const, override));

    // IFocusCapable implementation
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(types::FocusRange, getFocusLimits, (), (const, override));

    // IAutoFocusCapable implementation
    MOCK_METHOD(Result<void>, enableAutoFocus, (bool), (const, override));

    // IInfoCapable implementation
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));

    // IStabilizationCapable implementation
    MOCK_METHOD(Result<void>, stabilize, (bool), (const, override));
};

class MockRegisterImpl: public data::IRegisterImpl {
public:
    MOCK_METHOD(Result<void>, set, (uint32_t address, uint32_t value), (override));
    MOCK_METHOD(Result<uint32_t>, get, (uint32_t address), (const, override));
};