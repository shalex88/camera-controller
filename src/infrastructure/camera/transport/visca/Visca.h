#pragma once

#include <memory>

#include "infrastructure/camera/transport/ITransport.h"

using error_code = uint8_t;

inline constexpr uint8_t VISCA_START_BYTE = 0x80;
inline constexpr uint8_t VISCA_RESPONSE_START_BYTE = 0x90;
inline constexpr uint8_t VISCA_COMMAND = 0x01;
inline constexpr uint8_t VISCA_INQUIRY = 0x09;
inline constexpr uint8_t VISCA_TERMINATOR = 0xFF;
inline constexpr uint8_t VISCA_CATEGORY_INTERFACE = 0x00;
inline constexpr uint8_t VISCA_CATEGORY_CAMERA1 = 0x04;
inline constexpr uint8_t VISCA_CATEGORY_PAN_TILTER = 0x06;
inline constexpr uint8_t VISCA_CATEGORY_CAMERA2 = 0x07;

enum class CameraVendors : uint16_t {
    Sony = 0x0020
};

enum class CameraModels : uint16_t {
    EW9500H = 0x070F
};

// Commands/inquiries codes
inline constexpr uint8_t VISCA_POWER = 0x00;
inline constexpr uint8_t VISCA_ADDRESS = 0x30;
inline constexpr uint8_t VISCA_DEVICE_INFO = 0x02;
inline constexpr uint8_t VISCA_KEYLOCK = 0x17;
inline constexpr uint8_t VISCA_ID = 0x22;
inline constexpr uint8_t VISCA_ZOOM = 0x07;
inline constexpr uint8_t VISCA_ZOOM_STOP = 0x00;
inline constexpr uint8_t VISCA_ZOOM_TELE = 0x02;
inline constexpr uint8_t VISCA_ZOOM_WIDE = 0x03;
inline constexpr uint8_t VISCA_ZOOM_TELE_SPEED = 0x20;
inline constexpr uint8_t VISCA_ZOOM_WIDE_SPEED = 0x30;
inline constexpr uint8_t VISCA_ZOOM_VALUE = 0x47;
inline constexpr uint8_t VISCA_ZOOM_FOCUS_VALUE = 0x47;
inline constexpr uint8_t VISCA_DZOOM = 0x06;
inline constexpr uint8_t VISCA_DZOOM_VALUE = 0x46;
inline constexpr uint8_t VISCA_DZOOM_LIMIT = 0x26; /* implemented for H10 */
inline constexpr uint8_t VISCA_DZOOM_1X = 0x00;
inline constexpr uint8_t VISCA_DZOOM_1_5X = 0x01;
inline constexpr uint8_t VISCA_DZOOM_2X = 0x02;
inline constexpr uint8_t VISCA_DZOOM_4X = 0x03;
inline constexpr uint8_t VISCA_DZOOM_8X = 0x04;
inline constexpr uint8_t VISCA_DZOOM_12X = 0x05;
inline constexpr uint8_t VISCA_DZOOM_MODE = 0x36;
inline constexpr uint8_t VISCA_DZOOM_COMBINE = 0x00;
inline constexpr uint8_t VISCA_DZOOM_SEPARATE = 0x01;
inline constexpr uint8_t VISCA_FOCUS = 0x08;
inline constexpr uint8_t VISCA_FOCUS_STOP = 0x00;
inline constexpr uint8_t VISCA_FOCUS_FAR = 0x02;
inline constexpr uint8_t VISCA_FOCUS_NEAR = 0x03;
inline constexpr uint8_t VISCA_FOCUS_FAR_SPEED = 0x20;
inline constexpr uint8_t VISCA_FOCUS_NEAR_SPEED = 0x30;
inline constexpr uint8_t VISCA_FOCUS_VALUE = 0x48;
inline constexpr uint8_t VISCA_FOCUS_AUTO = 0x38;
inline constexpr uint8_t VISCA_FOCUS_AUTO_MAN = 0x10;
inline constexpr uint8_t VISCA_FOCUS_ONE_PUSH = 0x18;
inline constexpr uint8_t VISCA_FOCUS_ONE_PUSH_TRIG = 0x01;
inline constexpr uint8_t VISCA_FOCUS_ONE_PUSH_INF = 0x02;
inline constexpr uint8_t VISCA_FOCUS_AUTO_SENSE = 0x58;
inline constexpr uint8_t VISCA_FOCUS_AUTO_SENSE_HIGH = 0x02;
inline constexpr uint8_t VISCA_FOCUS_AUTO_SENSE_LOW = 0x03;
inline constexpr uint8_t VISCA_FOCUS_NEAR_LIMIT = 0x28;
inline constexpr uint8_t VISCA_WB = 0x35;
inline constexpr uint8_t VISCA_WB_AUTO = 0x00;
inline constexpr uint8_t VISCA_WB_INDOOR = 0x01;
inline constexpr uint8_t VISCA_WB_OUTDOOR = 0x02;
inline constexpr uint8_t VISCA_WB_ONE_PUSH = 0x03;
inline constexpr uint8_t VISCA_WB_ATW = 0x04;
inline constexpr uint8_t VISCA_WB_MANUAL = 0x05;
inline constexpr uint8_t VISCA_WB_TRIGGER = 0x10;
inline constexpr uint8_t VISCA_WB_ONE_PUSH_TRIG = 0x05;
inline constexpr uint8_t VISCA_RGAIN = 0x03;
inline constexpr uint8_t VISCA_RGAIN_VALUE = 0x43;
inline constexpr uint8_t VISCA_BGAIN = 0x04;
inline constexpr uint8_t VISCA_BGAIN_VALUE = 0x44;
inline constexpr uint8_t VISCA_AUTO_EXP = 0x39;
inline constexpr uint8_t VISCA_AUTO_EXP_FULL_AUTO = 0x00;
inline constexpr uint8_t VISCA_AUTO_EXP_MANUAL = 0x03;
inline constexpr uint8_t VISCA_AUTO_EXP_SHUTTER_PRIORITY = 0x0A;
inline constexpr uint8_t VISCA_AUTO_EXP_IRIS_PRIORITY = 0x0B;
inline constexpr uint8_t VISCA_AUTO_EXP_GAIN_PRIORITY = 0x0C;
inline constexpr uint8_t VISCA_AUTO_EXP_BRIGHT = 0x0D;
inline constexpr uint8_t VISCA_AUTO_EXP_SHUTTER_AUTO = 0x1A;
inline constexpr uint8_t VISCA_AUTO_EXP_IRIS_AUTO = 0x1B;
inline constexpr uint8_t VISCA_AUTO_EXP_GAIN_AUTO = 0x1C;
inline constexpr uint8_t VISCA_SLOW_SHUTTER = 0x5A;
inline constexpr uint8_t VISCA_SLOW_SHUTTER_AUTO = 0x02;
inline constexpr uint8_t VISCA_SLOW_SHUTTER_MANUAL = 0x03;
inline constexpr uint8_t VISCA_SHUTTER = 0x0A;
inline constexpr uint8_t VISCA_SHUTTER_VALUE = 0x4A;
inline constexpr uint8_t VISCA_IRIS = 0x0B;
inline constexpr uint8_t VISCA_IRIS_VALUE = 0x4B;
inline constexpr uint8_t VISCA_GAIN = 0x0C;
inline constexpr uint8_t VISCA_GAIN_VALUE = 0x4C;
inline constexpr uint8_t VISCA_BRIGHT = 0x0D;
inline constexpr uint8_t VISCA_BRIGHT_VALUE = 0x4D;
inline constexpr uint8_t VISCA_EXP_COMP = 0x0E;
inline constexpr uint8_t VISCA_EXP_COMP_POWER = 0x3E;
inline constexpr uint8_t VISCA_EXP_COMP_VALUE = 0x4E;
inline constexpr uint8_t VISCA_BACKLIGHT_COMP = 0x33;
inline constexpr uint8_t VISCA_SPOT_AE = 0x59;
inline constexpr uint8_t VISCA_SPOT_AE_POSITION = 0x29;
inline constexpr uint8_t VISCA_APERTURE = 0x02;
inline constexpr uint8_t VISCA_APERTURE_VALUE = 0x42;
inline constexpr uint8_t VISCA_ZERO_LUX = 0x01;
inline constexpr uint8_t VISCA_IR_LED = 0x31;
inline constexpr uint8_t VISCA_WIDE_MODE = 0x60;
inline constexpr uint8_t VISCA_WIDE_MODE_OFF = 0x00;
inline constexpr uint8_t VISCA_WIDE_MODE_CINEMA = 0x01;
inline constexpr uint8_t VISCA_WIDE_MODE_16_9 = 0x02;
inline constexpr uint8_t VISCA_MIRROR = 0x61;
inline constexpr uint8_t VISCA_FREEZE = 0x62;
inline constexpr uint8_t VISCA_PICTURE_EFFECT = 0x63;
inline constexpr uint8_t VISCA_PICTURE_EFFECT_OFF = 0x00;
inline constexpr uint8_t VISCA_PICTURE_EFFECT_PASTEL = 0x01;
inline constexpr uint8_t VISCA_PICTURE_EFFECT_NEGATIVE = 0x02;
inline constexpr uint8_t VISCA_PICTURE_EFFECT_SEPIA = 0x03;
inline constexpr uint8_t VISCA_PICTURE_EFFECT_BW = 0x04;
inline constexpr uint8_t VISCA_PICTURE_EFFECT_SOLARIZE = 0x05;
inline constexpr uint8_t VISCA_PICTURE_EFFECT_MOSAIC = 0x06;
inline constexpr uint8_t VISCA_PICTURE_EFFECT_SLIM = 0x07;
inline constexpr uint8_t VISCA_PICTURE_EFFECT_STRETCH = 0x08;
inline constexpr uint8_t VISCA_DIGITAL_EFFECT = 0x64;
inline constexpr uint8_t VISCA_DIGITAL_EFFECT_OFF = 0x00;
inline constexpr uint8_t VISCA_DIGITAL_EFFECT_STILL = 0x01;
inline constexpr uint8_t VISCA_DIGITAL_EFFECT_FLASH = 0x02;
inline constexpr uint8_t VISCA_DIGITAL_EFFECT_LUMI = 0x03;
inline constexpr uint8_t VISCA_DIGITAL_EFFECT_TRAIL = 0x04;
inline constexpr uint8_t VISCA_DIGITAL_EFFECT_LEVEL = 0x65;
inline constexpr uint8_t VISCA_CAM_STABILIZER = 0x34;
inline constexpr uint8_t VISCA_MEMORY = 0x3F;
inline constexpr uint8_t VISCA_MEMORY_RESET = 0x00;
inline constexpr uint8_t VISCA_MEMORY_SET = 0x01;
inline constexpr uint8_t VISCA_MEMORY_RECALL = 0x02;
inline constexpr uint8_t VISCA_MEMORY_0 = 0x00;
inline constexpr uint8_t VISCA_MEMORY_1 = 0x01;
inline constexpr uint8_t VISCA_MEMORY_2 = 0x02;
inline constexpr uint8_t VISCA_MEMORY_3 = 0x03;
inline constexpr uint8_t VISCA_MEMORY_4 = 0x04;
inline constexpr uint8_t VISCA_MEMORY_5 = 0x05;
inline constexpr uint8_t VISCA_MEMORY_CUSTOM = 0x7F;
inline constexpr uint8_t VISCA_DISPLAY = 0x15;
inline constexpr uint8_t VISCA_DISPLAY_TOGGLE = 0x10;
inline constexpr uint8_t VISCA_DATE_TIME_SET = 0x70;
inline constexpr uint8_t VISCA_DATE_DISPLAY = 0x71;
inline constexpr uint8_t VISCA_TIME_DISPLAY = 0x72;
inline constexpr uint8_t VISCA_TITLE_DISPLAY = 0x74;
inline constexpr uint8_t VISCA_TITLE_DISPLAY_CLEAR = 0x00;
inline constexpr uint8_t VISCA_TITLE_SET = 0x73;
inline constexpr uint8_t VISCA_TITLE_SET_PARAMS = 0x00;
inline constexpr uint8_t VISCA_TITLE_SET_PART1 = 0x01;
inline constexpr uint8_t VISCA_TITLE_SET_PART2 = 0x02;
inline constexpr uint8_t VISCA_IRRECEIVE = 0x08;
inline constexpr uint8_t VISCA_IRRECEIVE_ONOFF = 0x10;
inline constexpr uint8_t VISCA_PT_DRIVE = 0x01;
inline constexpr uint8_t VISCA_PT_DRIVE_HORIZ_LEFT = 0x01;
inline constexpr uint8_t VISCA_PT_DRIVE_HORIZ_RIGHT = 0x02;
inline constexpr uint8_t VISCA_PT_DRIVE_HORIZ_STOP = 0x03;
inline constexpr uint8_t VISCA_PT_DRIVE_VERT_UP = 0x01;
inline constexpr uint8_t VISCA_PT_DRIVE_VERT_DOWN = 0x02;
inline constexpr uint8_t VISCA_PT_DRIVE_VERT_STOP = 0x03;
inline constexpr uint8_t VISCA_PT_ABSOLUTE_POSITION = 0x02;
inline constexpr uint8_t VISCA_PT_RELATIVE_POSITION = 0x03;
inline constexpr uint8_t VISCA_PT_HOME = 0x04;
inline constexpr uint8_t VISCA_PT_RESET = 0x05;
inline constexpr uint8_t VISCA_PT_LIMITSET = 0x07;
inline constexpr uint8_t VISCA_PT_LIMITSET_SET = 0x00;
inline constexpr uint8_t VISCA_PT_LIMITSET_CLEAR = 0x01;
inline constexpr uint8_t VISCA_PT_LIMITSET_SET_UR = 0x01;
inline constexpr uint8_t VISCA_PT_LIMITSET_SET_DL = 0x00;
inline constexpr uint8_t VISCA_PT_DATASCREEN = 0x06;
inline constexpr uint8_t VISCA_PT_DATASCREEN_ONOFF = 0x10;
inline constexpr uint8_t VISCA_PT_VIDEOSYSTEM_INQ = 0x23;
inline constexpr uint8_t VISCA_PT_MODE_INQ = 0x10;
inline constexpr uint8_t VISCA_PT_MAXSPEED_INQ = 0x11;
inline constexpr uint8_t VISCA_PT_POSITION_INQ = 0x12;
inline constexpr uint8_t VISCA_PT_DATASCREEN_INQ = 0x06;
/**************************/
/* DIRECT REGISTER ACCESS */
/**************************/
inline constexpr uint8_t VISCA_REGISTER_VALUE = 0x24;
inline constexpr uint8_t VISCA_REGISTER_VISCA_BAUD = 0x00;
inline constexpr uint8_t VISCA_REGISTER_BD9600 = 0x00;
inline constexpr uint8_t VISCA_REGISTER_BD19200 = 0x01;
inline constexpr uint8_t VISCA_REGISTER_BD38400 = 0x02;
/* FCB-H10: Video Standard */
inline constexpr uint8_t VISCA_REGISTER_VIDEO_SIGNAL = 0x70;
inline constexpr uint8_t VISCA_REGISTER_VIDEO_1080I_60 = 0x01;
inline constexpr uint8_t VISCA_REGISTER_VIDEO_720P_60 = 0x02;
inline constexpr uint8_t VISCA_REGISTER_VIDEO_D1_CROP_60 = 0x03;
inline constexpr uint8_t VISCA_REGISTER_VIDEO_D1_SQ_60 = 0x04;
inline constexpr uint8_t VISCA_REGISTER_VIDEO_1080I_50 = 0x11;
inline constexpr uint8_t VISCA_REGISTER_VIDEO_720P_50 = 0x12;
inline constexpr uint8_t VISCA_REGISTER_VIDEO_D1_CROP_50 = 0x13;
inline constexpr uint8_t VISCA_REGISTER_VIDEO_D1_SQ_50 = 0x14;
/*****************/
/* D30/D31 CODES */
/*****************/
inline constexpr uint8_t VISCA_WIDE_CON_LENS = 0x26;
inline constexpr uint8_t VISCA_WIDE_CON_LENS_SET = 0x00;
inline constexpr uint8_t VISCA_AT_MODE = 0x01;
inline constexpr uint8_t VISCA_AT_ONOFF = 0x10;
inline constexpr uint8_t VISCA_AT_AE = 0x02;
inline constexpr uint8_t VISCA_AT_AUTOZOOM = 0x03;
inline constexpr uint8_t VISCA_ATMD_FRAMEDISPLAY = 0x04;
inline constexpr uint8_t VISCA_AT_FRAMEOFFSET = 0x05;
inline constexpr uint8_t VISCA_ATMD_STARTSTOP = 0x06;
inline constexpr uint8_t VISCA_AT_CHASE = 0x07;
inline constexpr uint8_t VISCA_AT_CHASE_NEXT = 0x10;
inline constexpr uint8_t VISCA_MD_MODE = 0x08;
inline constexpr uint8_t VISCA_MD_ONOFF = 0x10;
inline constexpr uint8_t VISCA_MD_FRAME = 0x09;
inline constexpr uint8_t VISCA_MD_DETECT = 0x0A;
inline constexpr uint8_t VISCA_MD_ADJUST = 0x00;
inline constexpr uint8_t VISCA_MD_ADJUST_YLEVEL = 0x0B;
inline constexpr uint8_t VISCA_MD_ADJUST_HUELEVEL = 0x0C;
inline constexpr uint8_t VISCA_MD_ADJUST_SIZE = 0x0D;
inline constexpr uint8_t VISCA_MD_ADJUST_DISPTIME = 0x0F;
inline constexpr uint8_t VISCA_MD_ADJUST_REFTIME = 0x0B;
inline constexpr uint8_t VISCA_MD_ADJUST_REFMODE = 0x10;
inline constexpr uint8_t VISCA_AT_ENTRY = 0x15;
inline constexpr uint8_t VISCA_AT_LOSTINFO = 0x20;
inline constexpr uint8_t VISCA_MD_LOSTINFO = 0x21;
inline constexpr uint8_t VISCA_ATMD_LOSTINFO1 = 0x20;
inline constexpr uint8_t VISCA_ATMD_LOSTINFO2 = 0x07;
inline constexpr uint8_t VISCA_MD_MEASURE_MODE_1 = 0x27;
inline constexpr uint8_t VISCA_MD_MEASURE_MODE_2 = 0x28;
inline constexpr uint8_t VISCA_ATMD_MODE = 0x22;
inline constexpr uint8_t VISCA_AT_MODE_QUERY = 0x23; // CAM_MemSave
inline constexpr uint8_t VISCA_MD_MODE_QUERY = 0x24;
inline constexpr uint8_t VISCA_MD_REFTIME_QUERY = 0x11;
inline constexpr uint8_t VISCA_AT_POSITION = 0x20;
inline constexpr uint8_t VISCA_MD_POSITION = 0x21;

/* Generic definitions */
inline constexpr uint8_t VISCA_ON = 0x02;
inline constexpr uint8_t VISCA_OFF = 0x03;
inline constexpr uint8_t VISCA_RESET = 0x00;
inline constexpr uint8_t VISCA_UP = 0x02;
inline constexpr uint8_t VISCA_DOWN = 0x03;

enum class ResponseType : uint32_t {
    Clear = 0x40,
    Address = 0x30,
    Ack = 0x40,
    Completed = 0x50,
    Error = 0x60
};

enum class ResultCode : error_code {
    Success = 0x00,
    Failure = 0xFF,
    ErrorMessageLength = 0x01, //ResponseType 0x60 | camera.address + Message length error (>14 bytes)
    ErrorSyntax = 0x02, //ResponseType 0x60 | camera.address + 0x02
    ErrorCmdBufferFull = 0x03, //ResponseType 0x60 | camera.address + ErrorCmdBufferFull
    ErrorCmdCancelled = 0x04, //ResponseType 0x60 | camera.address + ErrorCmdCancelled
    ErrorNoSocket = 0x05, //ResponseType 0x60 | camera.address + ErrorNoSocket
    ErrorCmdNotExecutable = 0x41 //ResponseType 0x60 | camera.address + ErrorCmdNotExecutable
};

/* timeout in us */
inline constexpr uint32_t VISCA_SERIAL_WAIT = 100000;
/* size of the local payload buffer */
inline constexpr uint32_t VISCA_INPUT_BUFFER_SIZE = 1024;
inline constexpr uint32_t VISCA_SERIAL_PACKET_SIZE = 16;

namespace camera_service::infrastructure {
    class Visca {
    public:
        struct ViscaTitleData {
            uint32_t vposition{};
            uint32_t hposition{};
            uint32_t color{};
            uint32_t blink{};
            std::array<uint8_t, 20> title{};
        };

        struct ViscaPayload { //different for Visca over IP
            std::array<uint8_t, VISCA_SERIAL_PACKET_SIZE> data{};
            size_t size = 0;
        };

        explicit Visca(std::unique_ptr<ITransport> transport);
        ~Visca() = default;

        Result<void> setAddress();
        Result<void> clear() const;
        Result<std::string_view> getCameraInfo();
        Result<void> open() const;
        Result<void> close() const;
        /* COMMANDS */
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
        /*  pan_speed should be in the range 01 - 18.
            tilt_speed should be in the range 01 - 14 */
        Result<void> setPanTiltUp(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltDown(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltLeft(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltRight(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltUpleft(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltUpright(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltDownleft(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltDownright(uint8_t pan_speed, uint8_t tilt_speed) const;
        Result<void> setPanTiltStop(uint8_t pan_speed, uint8_t tilt_speed) const;
        /*  pan_speed should be in the range 01 - 18.
            tilt_speed should be in the range 01 - 14
            pan_position should be in the range -880 - 880 (0xFC90 - 0x370)
            tilt_position should be in range -300 - 300 (0xFED4 - 0x12C)  */
        Result<void> setPanTiltAbsolutePosition(uint8_t pan_speed, uint8_t tilt_speed, uint16_t pan_position,
                                              uint16_t tilt_position) const;
        Result<void> setPanTiltRelativePosition(uint8_t pan_speed, uint8_t tilt_speed, uint16_t pan_position,
                                              uint16_t tilt_position) const;
        Result<void> setPanTiltHome() const;
        Result<void> setPanTiltReset() const;
        /*  pan_limit should be in the range -880 - 880 (0xFC90 - 0x370)
            tilt_limit should be in range -300 - 300 (0xFED4 - 0x12C)  */
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
        /* SPECIAL FUNCTIONS FOR D30/31 */
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
        uint8_t address_ = 0;
        uint8_t broadcast_ = 0;
        mutable std::array<uint8_t, 1024> rx_buffer_{};
        mutable uint32_t buffer_size_ = 0;
        uint8_t cam_address_ = 0;

        static std::string getViscaErrorMessage(ResultCode error_code);
        static void appendByte(ViscaPayload* payload, uint8_t byte);
        static void appendAsNibbles(ViscaPayload* payload, uint16_t value);
        static uint16_t get16BitFromNibbles(const ViscaPayload& payload, size_t index);
        static uint8_t get8Bit(const ViscaPayload& payload, size_t index);
        static uint8_t get8BitFromNibbles(const ViscaPayload& payload, size_t index);
        static uint16_t get16Bit(const ViscaPayload& payload, std::size_t index);
        Result<ResponseType> getReply() const;
        Result<ViscaPayload> sendAndReceiveReply(ViscaPayload* payload) const;
        Result<void> send(ViscaPayload* payload) const;
        ResultCode read() const;
        static std::string_view getCameraVendor(uint16_t vendor);
        static std::string_view getCameraModel(uint16_t model);
        static std::span<uint8_t> serialize(ViscaPayload* payload);
        static ViscaPayload deserialize(std::span<const uint8_t> buffer);
        std::vector<uint8_t> encode(std::span<const uint8_t> payload) const;
        std::vector<uint8_t> decode(std::span<const uint8_t> buffer) const;
    };
}
