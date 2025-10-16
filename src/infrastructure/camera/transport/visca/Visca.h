#pragma once

#include <cstdint>
#include <memory>
#include <termios.h>

// #include "infrastructure/camera/transport/ITransport.h"
#include "infrastructure/camera/transport/uart/Uart.h"

using error_code = uint8_t;

inline constexpr uint8_t VISCA_START_BYTE = 0x80;
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
    IX47X = 0x0401,
    EX47XL = 0x0402,
    IX10 = 0x0404,
    EX780 = 0x0411,
    EX480A = 0x0412,
    EX480AP = 0x0413,
    EX48Ax = 0x0414,
    EX45M = 0x041E,
    EX45MCE = 0x041F,
    IX47A = 0x0418,
    IX47AP = 0x0419,
    IX45A = 0x041A,
    IX45AP = 0x041B,
    IX10A = 0x041C,
    IX10AP = 0x041D,
    EX780B = 0x0420,
    EX780BP = 0x0421,
    EX78B = 0x0422,
    EX78BP = 0x0423,
    EX480B = 0x0424,
    EX480BP = 0x0425,
    EX48B = 0x0426,
    EX48BP = 0x0427,
    EX980S = 0x042E,
    EX980SP = 0x042F,
    EX980 = 0x0430,
    EX980P = 0x0431,
    EW9500H = 0x070F,
    H10 = 0x044A
};

// Commands/inquiries codes
inline constexpr uint8_t VISCA_POWER = 0x00;
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
inline constexpr uint8_t VISCA_AT_MODE_QUERY = 0x23;
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

enum class ResponseType : error_code {
    Clear = 0x40,
    Address = 0x30,
    Ack = 0x40, // | camera.address
    Completed = 0x50, // | camera.address
    Error = 0x60 // | camera.address
};

/* timeout in us */
inline constexpr uint32_t VISCA_SERIAL_WAIT = 100000;
/* size of the local packet buffer */
inline constexpr uint32_t VISCA_INPUT_BUFFER_SIZE = 1024;

namespace camera_service::infrastructure {
    class Visca {
    public:
        struct ViscaCamera {
            // VISCA data:
            int address;
            // camera info:
            uint16_t vendor;
            uint16_t model;
            uint16_t rom_version;
            uint8_t socket_num;
        };

        struct ViscaTitleData {
            uint32_t vposition;
            uint32_t hposition;
            uint32_t color;
            uint32_t blink;
            uint8_t title[20];
        };

        struct ViscaPacket {
            uint8_t bytes[16];
            uint32_t size = 1;
        };

        explicit Visca(std::unique_ptr<Uart> transport);
        ~Visca() = default;

        Result<void> setAddress();
        Result<void> clear() const;
        Result<std::string_view> getCameraInfo();
        Result<void> open() const;
        Result<void> close() const;
        /* COMMANDS */
        ResultCode setPower(uint8_t power) const;
        ResultCode setKeylock(uint8_t power) const;
        ResultCode setCameraId(uint16_t id) const;
        ResultCode setZoomTele() const;
        ResultCode setZoomWide() const;
        ResultCode setZoomStop() const;
        ResultCode setZoomTeleSpeed(uint32_t speed) const; //TODO: uint8_t?
        ResultCode setZoomWideSpeed(uint32_t speed) const; //TODO: uint8_t?
        Result<void> setZoomValue(uint16_t zoom) const;
        ResultCode setZoomAndFocusValue(uint16_t zoom, uint16_t focus) const;
        ResultCode setDzoomValue(uint8_t value) const;
        ResultCode setDzoomLimit(uint8_t limit) const;
        ResultCode setDzoomMode(uint8_t power) const;
        ResultCode setFocusFar() const;
        ResultCode setFocusNear() const;
        ResultCode setFocusStop() const;
        ResultCode setFocusFarSpeed(uint32_t speed) const; //TODO: uint8_t?
        ResultCode setFocusNearSpeed(uint32_t speed) const; //TODO: uint8_t?
        Result<void> setFocusValue(uint16_t focus) const;
        Result<void> setFocusAuto(bool on) const;
        ResultCode setFocusOnePush() const;
        ResultCode setFocusInfinity() const;
        ResultCode setFocusAutosenseHigh() const;
        ResultCode setFocusAutosenseLow() const;
        ResultCode setFocusNearLimit(uint16_t limit) const;
        ResultCode setWhitebalMode(uint8_t mode) const;
        ResultCode setWhitebalOnePush() const;
        ResultCode setRgainUp() const;
        ResultCode setRgainDown() const;
        ResultCode setRgainReset() const;
        ResultCode setRgainValue(uint8_t value) const;
        ResultCode setBgainUp() const;
        ResultCode setBgainDown() const;
        ResultCode setBgainReset() const;
        ResultCode setBgainValue(uint8_t value) const;
        ResultCode setShutterUp() const;
        ResultCode setShutterDown() const;
        ResultCode setShutterReset() const;
        ResultCode setShutterValue(uint8_t value) const;
        ResultCode setIrisUp() const;
        ResultCode setIrisDown() const;
        ResultCode setIrisReset() const;
        ResultCode setIrisValue(uint8_t value) const;
        ResultCode setGainUp() const;
        ResultCode setGainDown() const;
        ResultCode setGainReset() const;
        ResultCode setGainValue(uint8_t value) const;
        ResultCode setBrightUp() const;
        ResultCode setBrightDown() const;
        ResultCode setBrightReset() const;
        ResultCode setBrightValue(uint16_t value) const;
        ResultCode setApertureUp() const;
        ResultCode setApertureDown() const;
        ResultCode setApertureReset() const;
        ResultCode setApertureValue(uint16_t value) const;
        ResultCode setExpCompUp() const;
        ResultCode setExpCompDown() const;
        ResultCode setExpCompReset() const;
        ResultCode setExpCompValue(uint8_t value) const;
        ResultCode setExpCompPower(uint8_t power) const;
        ResultCode setAutoExpMode(uint8_t mode) const;
        ResultCode setSlowShutterAuto(uint8_t power) const;
        ResultCode setBacklightComp(uint8_t power) const;
        ResultCode setZeroLuxShot(uint8_t power) const;
        ResultCode setIrLed(uint8_t power) const;
        ResultCode setWideMode(uint8_t mode) const;
        ResultCode setMirror(uint8_t power) const;
        ResultCode setFreeze(uint8_t power) const;
        ResultCode setPictureEffect(uint8_t mode) const;
        ResultCode setDigitalEffect(uint8_t mode) const;
        ResultCode setDigitalEffectLevel(uint8_t level) const;
        Result<void> setCamStabilizer(bool power) const;
        ResultCode memorySet(uint8_t channel) const;
        ResultCode memoryRecall(uint8_t channel) const;
        ResultCode memoryReset(uint8_t channel) const;
        ResultCode setDisplay(uint8_t power) const;
        ResultCode setDateTime(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute) const;
        ResultCode setDateDisplay(uint8_t power) const;
        ResultCode setTimeDisplay(uint8_t power) const;
        ResultCode setTitleDisplay(uint8_t power) const;
        ResultCode setTitleClear() const;
        ResultCode setTitleParams(const ViscaTitleData* title) const;
        ResultCode setTitle(const ViscaTitleData* title) const;
        ResultCode setIrreceiveOn() const;
        ResultCode setIrreceiveOff() const;
        ResultCode setIrreceiveOnoff() const;
        /*  pan_speed should be in the range 01 - 18.
            tilt_speed should be in the range 01 - 14 */
        ResultCode setPanTiltUp(uint8_t pan_speed, uint8_t tilt_speed) const;
        ResultCode setPanTiltDown(uint8_t pan_speed, uint8_t tilt_speed) const;
        ResultCode setPanTiltLeft(uint8_t pan_speed, uint8_t tilt_speed) const;
        ResultCode setPanTiltRight(uint8_t pan_speed, uint8_t tilt_speed) const;
        ResultCode setPanTiltUpleft(uint8_t pan_speed, uint8_t tilt_speed) const;
        ResultCode setPanTiltUpright(uint8_t pan_speed, uint8_t tilt_speed) const;
        ResultCode setPanTiltDownleft(uint8_t pan_speed, uint8_t tilt_speed) const;
        ResultCode setPanTiltDownright(uint8_t pan_speed, uint8_t tilt_speed) const;
        ResultCode setPanTiltStop(uint8_t pan_speed, uint8_t tilt_speed) const;
        /*  pan_speed should be in the range 01 - 18.
            tilt_speed should be in the range 01 - 14
            pan_position should be in the range -880 - 880 (0xFC90 - 0x370)
            tilt_position should be in range -300 - 300 (0xFED4 - 0x12C)  */
        ResultCode setPanTiltAbsolutePosition(uint8_t pan_speed, uint8_t tilt_speed, uint16_t pan_position,
                                              uint16_t tilt_position) const;
        ResultCode setPanTiltRelativePosition(uint8_t pan_speed, uint8_t tilt_speed, uint16_t pan_position,
                                              uint16_t tilt_position) const;
        ResultCode setPanTiltHome() const;
        ResultCode setPanTiltReset() const;
        /*  pan_limit should be in the range -880 - 880 (0xFC90 - 0x370)
            tilt_limit should be in range -300 - 300 (0xFED4 - 0x12C)  */
        ResultCode setPanTiltLimitUpright(uint16_t pan_limit, uint16_t tilt_limit) const;
        ResultCode setPanTiltLimitDownleft(uint16_t pan_limit, uint16_t tilt_limit) const;
        ResultCode setPanTiltLimitDownleftClear() const;
        ResultCode setPanTiltLimitUprightClear() const;
        ResultCode setDatascreenOn() const;
        ResultCode setDatascreenOff() const;
        ResultCode setDatascreenOnoff() const;
        ResultCode setSpotAeOn() const;
        ResultCode setSpotAeOff() const;
        ResultCode setSpotAePosition(uint8_t x_position, uint8_t y_position) const;
        /* INQUIRIES */
        ResultCode getPower(uint8_t* power) const;
        ResultCode getDzoomValue(uint8_t* value) const;
        ResultCode getDzoomLimit(uint8_t* value) const;
        Result<uint16_t> getZoomValue() const;
        Result<bool> getFocusAuto() const;
        Result<uint16_t> getFocusValue() const;
        ResultCode getFocusAutoSense(uint8_t* mode) const;
        ResultCode getFocusNearLimit(uint16_t* value) const;
        ResultCode getWhitebalMode(uint8_t* mode) const;
        ResultCode getRgainValue(uint8_t* value) const;
        ResultCode getBgainValue(uint8_t* value) const;
        ResultCode getAutoExpMode(uint8_t* mode) const;
        ResultCode getSlowShutterAuto(uint8_t* mode) const;
        ResultCode getShutterValue(uint8_t* value) const;
        ResultCode getIrisValue(uint8_t* value) const;
        ResultCode getGainValue(uint8_t* value) const;
        ResultCode getBrightValue(uint16_t* value) const;
        ResultCode getExpCompPower(uint8_t* power) const;
        ResultCode getExpCompValue(uint16_t* value) const;
        ResultCode getBacklightComp(uint8_t* power) const;
        ResultCode getApertureValue(uint16_t* value) const;
        ResultCode getZeroLuxShot(uint8_t* power) const;
        ResultCode getIrLed(uint8_t* power) const;
        ResultCode getWideMode(uint8_t* mode) const;
        ResultCode getMirror(uint8_t* power) const;
        ResultCode getFreeze(uint8_t* power) const;
        ResultCode getPictureEffect(uint8_t* mode) const;
        ResultCode getDigitalEffect(uint8_t* mode) const;
        ResultCode getDigitalEffectLevel(uint16_t* value) const;
        ResultCode getMemory(uint8_t* channel) const;
        ResultCode getDisplay(uint8_t* power) const;
        ResultCode getId(uint16_t* id) const;
        ResultCode getVideoSystem(uint8_t* system) const;
        ResultCode getPanTiltMode(uint16_t* status) const;
        ResultCode getPanTiltMaxspeed(uint8_t* max_pan_speed, uint8_t* max_tilt_speed) const;
        ResultCode getPanTiltPosition(uint16_t* pan_position, uint16_t* tilt_position) const;
        ResultCode getDatascreen(uint8_t* status) const;
        /* SPECIAL FUNCTIONS FOR D30/31 */
        ResultCode setWideConLens(uint8_t power) const;
        ResultCode setAtModeOnoff() const;
        ResultCode setAtMode(uint8_t power) const;
        ResultCode setAtAeOnoff() const;
        ResultCode setAtAe(uint8_t power) const;
        ResultCode setAtAutozoomOnoff() const;
        ResultCode setAtAutozoom(uint8_t power) const;
        ResultCode setAtmdFramedisplayOnoff() const;
        ResultCode setAtmdFramedisplay(uint8_t power) const;
        ResultCode setAtFrameoffsetOnoff() const;
        ResultCode setAtFrameoffset(uint8_t power) const;
        ResultCode setAtmdStartstop() const;
        ResultCode setAtChase(uint8_t power) const;
        ResultCode setAtChaseNext() const;
        ResultCode setMdModeOnoff() const;
        ResultCode setMdMode(uint8_t power) const;
        ResultCode setMdFrame() const;
        ResultCode setMdDetect() const;
        ResultCode setAtEntry(uint8_t power) const;
        ResultCode setAtLostinfo() const;
        ResultCode setMdLostinfo() const;
        ResultCode setMdAdjustYlevel(uint8_t power) const;
        ResultCode setMdAdjustHuelevel(uint8_t power) const;
        ResultCode setMdAdjustSize(uint8_t power) const;
        ResultCode setMdAdjustDisptime(uint8_t power) const;
        ResultCode setMdAdjustRefmode(uint8_t power) const;
        ResultCode setMdAdjustReftime(uint8_t power) const;
        ResultCode setMdMeasureMode1Onoff() const;
        ResultCode setMdMeasureMode1(uint8_t power) const;
        ResultCode setMdMeasureMode2Onoff() const;
        ResultCode setMdMeasureMode2(uint8_t power) const;
        ResultCode getKeylock(uint8_t* power) const;
        ResultCode getWideConLens(uint8_t* power) const;
        ResultCode getAtmdMode(uint8_t* power) const;
        ResultCode getAtMode(uint16_t* value) const;
        ResultCode getAtEntry(uint8_t* power) const;
        ResultCode getMdMode(uint16_t* value) const;
        ResultCode getMdYlevel(uint8_t* power) const;
        ResultCode getMdHuelevel(uint8_t* power) const;
        ResultCode getMdSize(uint8_t* power) const;
        ResultCode getMdDisptime(uint8_t* power) const;
        ResultCode getMdRefmode(uint8_t* power) const;
        ResultCode getMdReftime(uint8_t* power) const;
        ResultCode getAtObjPos(uint8_t* xpos, uint8_t* ypos, uint8_t* status) const;
        ResultCode getMdObjPos(uint8_t* xpos, uint8_t* ypos, uint8_t* status) const;
        ResultCode setRegister(uint8_t reg_num, uint8_t reg_val) const;
        ResultCode getRegister(uint8_t reg_num, uint8_t* reg_val) const;

    private:
        std::unique_ptr<Uart> transport_; //TODO: use ITransport
        ViscaCamera camera{};
        static std::string getViscaErrorMessage(ResultCode error_code);
        static void appendByte(ViscaPacket* packet, uint8_t byte);
        static void appendAsNibbles(ViscaPacket* packet, uint16_t value);
        uint16_t getFromNibbles() const;
        uint8_t getByte() const;
        ResultCode getReply() const;
        ResultCode sendPacketWithReply(ViscaPacket* packet) const;
        ResultCode write(const ViscaPacket* packet) const;
        ResultCode sendPacket(ViscaPacket* packet) const;
        ResultCode read() const;
        ResultCode unreadBytes(const uint8_t* buffer, size_t* buffer_size) const;
        static std::string_view getCameraVendor(uint16_t vendor);
        static std::string_view getCameraModel(uint16_t model);
    };
}
