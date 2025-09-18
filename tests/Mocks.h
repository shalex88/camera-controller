#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "api/ITransport.h"
#include "api/IRequestHandler.h"
#include "core/ICore.h"
#include "data/ICameraHal.h"
#include "data/ICameraHw.h"
#include "data/hw_interface/mmio/IRegisterImpl.h"

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
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));
    MOCK_METHOD(Result<void>, setMinZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setMaxZoom, (), (const, override));
};

class CoreMock: public core::ICore {
public:
    MOCK_METHOD(Result<void>, initialize, (), (override));
    MOCK_METHOD(Result<void>, shutdown, (), (override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));
    MOCK_METHOD(Result<void>, setMinZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setMaxZoom, (), (const, override));
};

class MockCameraHal : public data::ICameraHal {
public:
    MOCK_METHOD(Result<void>, connect, (), (override));
    MOCK_METHOD(Result<void>, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));
    MOCK_METHOD(Result<void>, setMinZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setMaxZoom, (), (const, override));
};

class MockCameraHw: public data::ICameraHw {
public:
    MOCK_METHOD(Result<void>, connect, (), (override));
    MOCK_METHOD(Result<void>, disconnect, (), (override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(types::CameraLimits, getLimits, (), (const, override));
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));
    MOCK_METHOD(Result<void>, setMinZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setMaxZoom, (), (const, override));

    types::CameraLimits limits {
        .min_zoom = 0,
        .max_zoom = 100,
        .min_focus = 0,
        .max_focus = 100,
    };
};

class MockRegisterImpl: public data::IRegisterImpl {
public:
    MOCK_METHOD(Result<void>, set, (uint32_t address, uint32_t value), (override));
    MOCK_METHOD(Result<uint32_t>, get, (uint32_t address), (const, override));
};