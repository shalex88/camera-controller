#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>

#include "common/types/Result.h"

namespace camera_service::infrastructure {
    class ITransport;

    class ViscaProtocol {
    public:
        struct ViscaPayload;
        struct ViscaTitleData {
            uint32_t vposition{};
            uint32_t hposition{};
            uint32_t color{};
            uint32_t blink{};
            std::array<std::byte, 20> title{};
        };

        explicit ViscaProtocol(std::unique_ptr<ITransport> transport);
        ~ViscaProtocol();

        Result<void> setAddress();
        Result<void> clear() const;
        Result<std::string_view> getCameraInfo() const;
        Result<void> open() const;
        Result<void> close() const;
        Result<void> setPower(uint8_t power) const;
        Result<void> setKeylock(uint8_t power) const;
        Result<void> setCameraId(uint16_t id) const;
        Result<void> setZoomTele() const;
        Result<void> setZoomWide() const;
        Result<void> setZoomStop() const;
        Result<void> setZoomTeleSpeed(uint32_t speed) const; //TODO: uint8_t?
        Result<void> setZoomWideSpeed(uint32_t speed) const; //TODO: uint8_t?
        Result<void> setZoomValue(uint16_t zoom) const;
        Result<void> setZoomAndFocusValue(uint16_t zoom, uint16_t focus) const;
        Result<void> setDzoomValue(uint8_t value) const;
        Result<void> setDzoomLimit(uint8_t limit) const;
        Result<void> setDzoomMode(uint8_t power) const;
        Result<void> setFocusFar() const;
        Result<void> setFocusNear() const;
        Result<void> setFocusStop() const;
        Result<void> setFocusFarSpeed(uint32_t speed) const; //TODO: uint8_t?
        Result<void> setFocusNearSpeed(uint32_t speed) const; //TODO: uint8_t?
        Result<void> setFocusValue(uint16_t focus) const;
        Result<void> setFocusAuto(bool on) const;
        Result<void> setFocusOnePush() const;
        Result<void> setFocusInfinity() const;
        Result<void> setFocusAutosenseHigh() const;
        Result<void> setFocusAutosenseLow() const;
        Result<void> setFocusNearLimit(uint16_t limit) const;
        Result<void> setWhitebalMode(uint8_t mode) const;
        Result<void> setWhitebalOnePush() const;
        Result<void> setRgainUp() const;
        Result<void> setRgainDown() const;
        Result<void> setRgainReset() const;
        Result<void> setRgainValue(uint8_t value) const;
        Result<void> setBgainUp() const;
        Result<void> setBgainDown() const;
        Result<void> setBgainReset() const;
        Result<void> setBgainValue(uint8_t value) const;
        Result<void> setShutterUp() const;
        Result<void> setShutterDown() const;
        Result<void> setShutterReset() const;
        Result<void> setShutterValue(uint8_t value) const;
        Result<void> setIrisUp() const;
        Result<void> setIrisDown() const;
        Result<void> setIrisReset() const;
        Result<void> setIrisValue(uint8_t value) const;
        Result<void> setGainUp() const;
        Result<void> setGainDown() const;
        Result<void> setGainReset() const;
        Result<void> setGainValue(uint8_t value) const;
        Result<void> setBrightUp() const;
        Result<void> setBrightDown() const;
        Result<void> setBrightReset() const;
        Result<void> setBrightValue(uint16_t value) const; //TODO: uint8_t?
        Result<void> setApertureUp() const;
        Result<void> setApertureDown() const;
        Result<void> setApertureReset() const;
        Result<void> setApertureValue(uint8_t value) const;
        Result<void> setExpCompUp() const;
        Result<void> setExpCompDown() const;
        Result<void> setExpCompReset() const;
        Result<void> setExpCompValue(uint8_t value) const;
        Result<void> setExpCompPower(uint8_t power) const;
        Result<void> setAutoExpMode(uint8_t mode) const;
        Result<void> setSlowShutterAuto(uint8_t power) const;
        Result<void> setBacklightComp(bool on) const;
        Result<void> setZeroLuxShot(uint8_t power) const;
        Result<void> setIrLed(uint8_t power) const;
        Result<void> setWideMode(uint8_t mode) const;
        Result<void> setMirror(uint8_t power) const;
        Result<void> setFreeze(uint8_t power) const;
        Result<void> setPictureEffect(uint8_t mode) const;
        Result<void> setDigitalEffect(uint8_t mode) const;
        Result<void> setDigitalEffectLevel(uint8_t level) const;
        Result<void> setCamStabilizer(bool power) const;
        Result<void> memorySet(uint8_t channel) const;
        Result<void> memoryRecall(uint8_t channel) const;
        Result<void> memoryReset(uint8_t channel) const;
        Result<void> setDisplay(uint8_t power) const;
        Result<void> setDateTime(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute) const;
        Result<void> setDateDisplay(uint8_t power) const;
        Result<void> setTimeDisplay(uint8_t power) const;
        Result<void> setTitleDisplay(uint8_t power) const;
        Result<void> setTitleClear() const;
        Result<void> setTitleParams(const ViscaTitleData* title) const;
        Result<void> setTitle(const ViscaTitleData* title) const;
        Result<void> setIrreceiveOn() const;
        Result<void> setIrreceiveOff() const;
        Result<void> setIrreceiveOnoff() const;
        Result<void> setPanTiltUp(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltDown(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltLeft(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltRight(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltUpleft(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltUpright(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltDownleft(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltDownright(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltStop(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltAbsolutePosition(uint8_t pan_speed, uint8_t tilt_speed, uint16_t pan_position,
                                                uint16_t tilt_position) const;
        Result<void> setPanTiltRelativePosition(uint8_t pan_speed, uint8_t tilt_speed, uint16_t pan_position,
                                                uint16_t tilt_position) const;
        Result<void> setPanTiltHome() const;
        Result<void> setPanTiltReset() const;
        Result<void> setPanTiltLimitUpright(uint16_t pan_limit, uint16_t tilt_limit) const;
        Result<void> setPanTiltLimitDownleft(uint16_t pan_limit, uint16_t tilt_limit) const;
        Result<void> setPanTiltLimitDownleftClear() const;
        Result<void> setPanTiltLimitUprightClear() const;
        Result<void> setDatascreenOn() const;
        Result<void> setDatascreenOff() const;
        Result<void> setDatascreenOnoff() const;
        Result<void> setSpotAeOn() const;
        Result<void> setSpotAeOff() const;
        Result<void> setSpotAePosition(uint8_t x_position, uint8_t y_position) const;
        Result<uint8_t> getPower() const;
        Result<uint8_t> getDzoomValue() const;
        Result<uint8_t> getDzoomLimit() const;
        Result<uint16_t> getZoomValue() const;
        Result<bool> getFocusAuto() const;
        Result<uint16_t> getFocusValue() const;
        Result<uint8_t> getFocusAutoSense() const;
        Result<uint16_t> getFocusNearLimit() const;
        Result<uint8_t> getWhitebalMode() const;
        Result<uint8_t> getRgainValue() const;
        Result<uint8_t> getBgainValue() const;
        Result<uint8_t> getAutoExpMode() const;
        Result<uint8_t> getSlowShutterAuto() const;
        Result<uint8_t> getShutterValue() const;
        Result<uint8_t> getIrisValue() const;
        Result<uint8_t> getGainValue() const;
        Result<uint16_t> getBrightValue() const; //TODO: uint8_t?
        Result<uint8_t> getExpCompPower() const;
        Result<uint8_t> getExpCompValue() const;
        Result<bool> getBacklightComp() const;
        Result<uint8_t> getApertureValue() const;
        Result<uint8_t> getZeroLuxShot() const;
        Result<uint8_t> getIrLed() const;
        Result<uint8_t> getWideMode() const;
        Result<uint8_t> getMirror() const;
        Result<uint8_t> getFreeze() const;
        Result<uint8_t> getPictureEffect() const;
        Result<uint8_t> getDigitalEffect() const;
        Result<uint16_t> getDigitalEffectLevel() const;
        Result<uint8_t> getMemory() const;
        Result<uint8_t> getDisplay() const;
        Result<uint16_t> getId() const;
        Result<uint8_t> getVideoSystem() const;
        Result<uint16_t> getPanTiltMode() const;
        Result<std::pair<uint8_t, uint8_t>> getPanTiltMaxspeed() const;
        Result<std::pair<uint16_t, uint16_t>> getPanTiltPosition() const;
        Result<uint8_t> getDatascreen() const;
        Result<void> setWideConLens(uint8_t power) const;
        Result<void> setAtModeOnoff() const;
        Result<void> setAtMode(uint8_t power) const;
        Result<void> setAtAeOnoff() const;
        Result<void> setAtAe(uint8_t power) const;
        Result<void> setAtAutozoomOnoff() const;
        Result<void> setAtAutozoom(uint8_t power) const;
        Result<void> setAtmdFramedisplayOnoff() const;
        Result<void> setAtmdFramedisplay(uint8_t power) const;
        Result<void> setAtFrameoffsetOnoff() const;
        Result<void> setAtFrameoffset(uint8_t power) const;
        Result<void> setAtmdStartstop() const;
        Result<void> setAtChase(uint8_t power) const;
        Result<void> setAtChaseNext() const;
        Result<void> setMdModeOnoff() const;
        Result<void> setMdMode(uint8_t power) const;
        Result<void> setMdFrame() const;
        Result<void> setMdDetect() const;
        Result<void> setAtEntry(uint8_t power) const;
        Result<void> setAtLostinfo() const;
        Result<void> setMdLostinfo() const;
        Result<void> setMdAdjustYlevel(uint8_t power) const;
        Result<void> setMdAdjustHuelevel(uint8_t power) const;
        Result<void> setMdAdjustSize(uint8_t power) const;
        Result<void> setMdAdjustDisptime(uint8_t power) const;
        Result<void> setMdAdjustRefmode(uint8_t power) const;
        Result<void> setMdAdjustReftime(uint8_t power) const;
        Result<void> setMdMeasureMode1Onoff() const;
        Result<void> setMdMeasureMode1(uint8_t power) const;
        Result<void> setMdMeasureMode2Onoff() const;
        Result<void> setMdMeasureMode2(uint8_t power) const;
        Result<uint8_t> getKeylock() const;
        Result<uint8_t> getWideConLens() const;
        Result<uint8_t> getAtmdMode() const;
        Result<uint16_t> getAtMode() const;
        Result<uint8_t> getAtEntry() const;
        Result<uint16_t> getMdMode() const;
        Result<uint8_t> getMdYlevel() const;
        Result<uint8_t> getMdHuelevel() const;
        Result<uint8_t> getMdSize() const;
        Result<uint8_t> getMdDisptime() const;
        Result<uint8_t> getMdRefmode() const;
        Result<uint8_t> getMdReftime() const;
        Result<void> setRegister(uint8_t reg_num, uint8_t reg_val) const;
        Result<uint8_t> getRegister(uint8_t reg_num) const;

    private:
        std::unique_ptr<ITransport> transport_;
        uint8_t broadcast_{};
        uint8_t cam_address_{};
        Result<ViscaPayload> sendAndReceiveReply(ViscaPayload* payload) const;
        std::vector<std::byte> encode(std::span<const std::byte> payload) const;
    };
}
