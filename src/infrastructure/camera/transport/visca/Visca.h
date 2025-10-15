#pragma once

#include <cstdint>
#include <termios.h>

using error_code = uint32_t;

inline constexpr uint8_t VISCA_COMMAND = 0x01;
inline constexpr uint8_t VISCA_INQUIRY = 0x09;
inline constexpr uint8_t VISCA_TERMINATOR = 0xFF;
inline constexpr uint8_t VISCA_CATEGORY_INTERFACE = 0x00;
inline constexpr uint8_t VISCA_CATEGORY_CAMERA1 = 0x04;
inline constexpr uint8_t VISCA_CATEGORY_PAN_TILTER = 0x06;
inline constexpr uint8_t VISCA_CATEGORY_CAMERA2 = 0x07;

// Vendor IDs
inline constexpr uint16_t VISCA_VENDOR_SONY = 0x0020;

// Model IDs
inline constexpr uint16_t VISCA_MODEL_IX47X = 0x0401;
inline constexpr uint16_t VISCA_MODEL_EX47XL = 0x0402;
inline constexpr uint16_t VISCA_MODEL_IX10 = 0x0404;
inline constexpr uint16_t VISCA_MODEL_EX780 = 0x0411;
inline constexpr uint16_t VISCA_MODEL_EX480A = 0x0412;
inline constexpr uint16_t VISCA_MODEL_EX480AP = 0x0413;
inline constexpr uint16_t VISCA_MODEL_EX48A = 0x0414;
inline constexpr uint16_t VISCA_MODEL_EX48AP = 0x0414;
inline constexpr uint16_t VISCA_MODEL_EX45M = 0x041E;
inline constexpr uint16_t VISCA_MODEL_EX45MCE = 0x041F;
inline constexpr uint16_t VISCA_MODEL_IX47A = 0x0418;
inline constexpr uint16_t VISCA_MODEL_IX47AP = 0x0419;
inline constexpr uint16_t VISCA_MODEL_IX45A = 0x041A;
inline constexpr uint16_t VISCA_MODEL_IX45AP = 0x041B;
inline constexpr uint16_t VISCA_MODEL_IX10A = 0x041C;
inline constexpr uint16_t VISCA_MODEL_IX10AP = 0x041D;
inline constexpr uint16_t VISCA_MODEL_EX780B = 0x0420;
inline constexpr uint16_t VISCA_MODEL_EX780BP = 0x0421;
inline constexpr uint16_t VISCA_MODEL_EX78B = 0x0422;
inline constexpr uint16_t VISCA_MODEL_EX78BP = 0x0423;
inline constexpr uint16_t VISCA_MODEL_EX480B = 0x0424;
inline constexpr uint16_t VISCA_MODEL_EX480BP = 0x0425;
inline constexpr uint16_t VISCA_MODEL_EX48B = 0x0426;
inline constexpr uint16_t VISCA_MODEL_EX48BP = 0x0427;
inline constexpr uint16_t VISCA_MODEL_EX980S = 0x042E;
inline constexpr uint16_t VISCA_MODEL_EX980SP = 0x042F;
inline constexpr uint16_t VISCA_MODEL_EX980 = 0x0430;
inline constexpr uint16_t VISCA_MODEL_EX980P = 0x0431;
inline constexpr uint16_t VISCA_MODEL_H10 = 0x044A;

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
inline constexpr uint8_t VISCA_DZOOM_OFF = 0x03;
inline constexpr uint8_t VISCA_DZOOM_ON = 0x02;
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
inline constexpr uint8_t VISCA_FOCUS_AUTO_ON = 0x02;
inline constexpr uint8_t VISCA_FOCUS_AUTO_OFF = 0x03;
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
inline constexpr uint8_t VISCA_SPOT_AE_ON = 0x02;
inline constexpr uint8_t VISCA_SPOT_AE_OFF = 0x03;
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
inline constexpr uint8_t VISCA_FREEZE_ON = 0x02;
inline constexpr uint8_t VISCA_FREEZE_OFF = 0x03;
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
inline constexpr uint8_t VISCA_CAM_STABILIZER_ON = 0x02;
inline constexpr uint8_t VISCA_CAM_STABILIZER_OFF = 0x03;
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
inline constexpr uint8_t VISCA_DISPLAY_ON = 0x02;
inline constexpr uint8_t VISCA_DISPLAY_OFF = 0x03;
inline constexpr uint8_t VISCA_DISPLAY_TOGGLE = 0x10;
inline constexpr uint8_t VISCA_DATE_TIME_SET = 0x70;
inline constexpr uint8_t VISCA_DATE_DISPLAY = 0x71;
inline constexpr uint8_t VISCA_TIME_DISPLAY = 0x72;
inline constexpr uint8_t VISCA_TITLE_DISPLAY = 0x74;
inline constexpr uint8_t VISCA_TITLE_DISPLAY_CLEAR = 0x00;
inline constexpr uint8_t VISCA_TITLE_DISPLAY_ON = 0x02;
inline constexpr uint8_t VISCA_TITLE_DISPLAY_OFF = 0x03;
inline constexpr uint8_t VISCA_TITLE_SET = 0x73;
inline constexpr uint8_t VISCA_TITLE_SET_PARAMS = 0x00;
inline constexpr uint8_t VISCA_TITLE_SET_PART1 = 0x01;
inline constexpr uint8_t VISCA_TITLE_SET_PART2 = 0x02;
inline constexpr uint8_t VISCA_IRRECEIVE = 0x08;
inline constexpr uint8_t VISCA_IRRECEIVE_ON = 0x02;
inline constexpr uint8_t VISCA_IRRECEIVE_OFF = 0x03;
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
inline constexpr uint8_t VISCA_PT_DATASCREEN_ON = 0x02;
inline constexpr uint8_t VISCA_PT_DATASCREEN_OFF = 0x03;
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

enum class ErrorCode : error_code {
    Success = 0x00,
    Failure = 0xFF,
    ErrorMessageLength = 0x01,
    ErrorSyntax = 0x02,
    ErrorCmdBufferFull = 0x03,
    ErrorCmdCancelled = 0x04,
    ErrorNoSocket = 0x05,
    ErrorCmdNotExecutable = 0x41
};

enum class ResponseType : error_code {
    Clear = 0x40,
    Address = 0x30,
    Ack = 0x40,
    Completed = 0x50,
    Error = 0x60
};

/* timeout in us */
inline constexpr uint32_t VISCA_SERIAL_WAIT = 100000;
/* size of the local packet buffer */
inline constexpr uint32_t VISCA_INPUT_BUFFER_SIZE = 1024;

namespace camera_service::infrastructure {
    class Visca {
    public:
        struct ViscaInterface {
            // RS232 data:
            int port_fd;
            termios options;
            uint32_t baud;
            // VISCA data:
            uint32_t address;
            uint32_t broadcast;
            // RS232 input buffer
            uint8_t ibuf[VISCA_INPUT_BUFFER_SIZE];
            uint32_t bytes;
            ResponseType type;
        };

        struct ViscaCamera {
            // VISCA data:
            int address;
            // camera info:
            uint32_t vendor;
            uint32_t model;
            uint32_t rom_version;
            uint32_t socket_num;
        };

        struct ViscaTitleData {
            uint32_t vposition;
            uint32_t hposition;
            uint32_t color;
            uint32_t blink;
            uint8_t title[20];
        };

        struct ViscaPacket {
            uint8_t bytes[32];
            uint32_t length;
        };

        Visca() = default;
        ~Visca() = default;

        ErrorCode setAddress();
        ErrorCode clear();
        ErrorCode getCameraInfo();
        ErrorCode open(const char* device_name);
        ErrorCode close();
        /* COMMANDS */
        ErrorCode setPower(uint8_t power);
        ErrorCode setKeylock(uint8_t power);
        ErrorCode setCameraId(uint16_t id);
        ErrorCode setZoomTele();
        ErrorCode setZoomWide();
        ErrorCode setZoomStop();
        ErrorCode setZoomTeleSpeed(uint32_t speed);
        ErrorCode setZoomWideSpeed(uint32_t speed);
        ErrorCode setZoomValue(uint32_t zoom);
        ErrorCode setZoomAndFocusValue(uint32_t zoom, uint32_t focus);
        ErrorCode setDzoom(uint32_t power);
        ErrorCode setDzoomLimit(uint32_t limit);
        ErrorCode setDzoomMode(uint32_t power);
        ErrorCode setFocusFar();
        ErrorCode setFocusNear();
        ErrorCode setFocusStop();
        ErrorCode setFocusFarSpeed(uint32_t speed);
        ErrorCode setFocusNearSpeed(uint32_t speed);
        ErrorCode setFocusValue(uint32_t focus);
        ErrorCode setFocusAuto(bool on);
        ErrorCode setFocusOnePush();
        ErrorCode setFocusInfinity();
        ErrorCode setFocusAutosenseHigh();
        ErrorCode setFocusAutosenseLow();
        ErrorCode setFocusNearLimit(uint32_t limit);
        ErrorCode setWhitebalMode(uint32_t mode);
        ErrorCode setWhitebalOnePush();
        ErrorCode setRgainUp();
        ErrorCode setRgainDown();
        ErrorCode setRgainReset();
        ErrorCode setRgainValue(uint32_t value);
        ErrorCode setBgainUp();
        ErrorCode setBgainDown();
        ErrorCode setBgainReset();
        ErrorCode setBgainValue(uint32_t value);
        ErrorCode setShutterUp();
        ErrorCode setShutterDown();
        ErrorCode setShutterReset();
        ErrorCode setShutterValue(uint32_t value);
        ErrorCode setIrisUp();
        ErrorCode setIrisDown();
        ErrorCode setIrisReset();
        ErrorCode setIrisValue(uint32_t value);
        ErrorCode setGainUp();
        ErrorCode setGainDown();
        ErrorCode setGainReset();
        ErrorCode setGainValue(uint32_t value);
        ErrorCode setBrightUp();
        ErrorCode setBrightDown();
        ErrorCode setBrightReset();
        ErrorCode setBrightValue(uint32_t value);
        ErrorCode setApertureUp();
        ErrorCode setApertureDown();
        ErrorCode setApertureReset();
        ErrorCode setApertureValue(uint32_t value);
        ErrorCode setExpCompUp();
        ErrorCode setExpCompDown();
        ErrorCode setExpCompReset();
        ErrorCode setExpCompValue(uint32_t value);
        ErrorCode setExpCompPower(uint8_t power);
        ErrorCode setAutoExpMode(uint8_t mode);
        ErrorCode setSlowShutterAuto(uint8_t power);
        ErrorCode setBacklightComp(uint8_t power);
        ErrorCode setZeroLuxShot(uint8_t power);
        ErrorCode setIrLed(uint8_t power);
        ErrorCode setWideMode(uint8_t mode);
        ErrorCode setMirror(uint8_t power);
        ErrorCode setFreeze(uint8_t power);
        ErrorCode setPictureEffect(uint8_t mode);
        ErrorCode setDigitalEffect(uint8_t mode);
        ErrorCode setDigitalEffectLevel(uint8_t level);
        ErrorCode setCamStabilizer(uint8_t power);
        ErrorCode memorySet(uint8_t channel);
        ErrorCode memoryRecall(uint8_t channel);
        ErrorCode memoryReset(uint8_t channel);
        ErrorCode setDisplay(uint8_t power);
        ErrorCode setDateTime(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t minute);
        ErrorCode setDateDisplay(uint8_t power);
        ErrorCode setTimeDisplay(uint8_t power);
        ErrorCode setTitleDisplay(uint8_t power);
        ErrorCode setTitleClear();
        ErrorCode setTitleParams(const ViscaTitleData* title);
        ErrorCode setTitle(const ViscaTitleData* title);
        ErrorCode setIrreceiveOn();
        ErrorCode setIrreceiveOff();
        ErrorCode setIrreceiveOnoff();
        /*  pan_speed should be in the range 01 - 18.
            tilt_speed should be in the range 01 - 14 */
        ErrorCode setPantiltUp(uint32_t pan_speed, uint32_t tilt_speed);
        ErrorCode setPantiltDown(uint32_t pan_speed, uint32_t tilt_speed);
        ErrorCode setPantiltLeft(uint32_t pan_speed, uint32_t tilt_speed);
        ErrorCode setPantiltRight(uint32_t pan_speed, uint32_t tilt_speed);
        ErrorCode setPantiltUpleft(uint32_t pan_speed, uint32_t tilt_speed);
        ErrorCode setPantiltUpright(uint32_t pan_speed, uint32_t tilt_speed);
        ErrorCode setPantiltDownleft(uint32_t pan_speed, uint32_t tilt_speed);
        ErrorCode setPantiltDownright(uint32_t pan_speed, uint32_t tilt_speed);
        ErrorCode setPantiltStop(uint32_t pan_speed, uint32_t tilt_speed);
        /*  pan_speed should be in the range 01 - 18.
            tilt_speed should be in the range 01 - 14
            pan_position should be in the range -880 - 880 (0xFC90 - 0x370)
            tilt_position should be in range -300 - 300 (0xFED4 - 0x12C)  */
        ErrorCode setPantiltAbsolutePosition(uint32_t pan_speed, uint32_t tilt_speed, uint32_t pan_position,
                                             uint32_t tilt_position);
        ErrorCode setPantiltRelativePosition(uint32_t pan_speed, uint32_t tilt_speed, uint32_t pan_position,
                                             uint32_t tilt_position);
        ErrorCode setPantiltHome();
        ErrorCode setPantiltReset();
        /*  pan_limit should be in the range -880 - 880 (0xFC90 - 0x370)
            tilt_limit should be in range -300 - 300 (0xFED4 - 0x12C)  */
        ErrorCode setPantiltLimitUpright(uint32_t pan_limit, uint32_t tilt_limit);
        ErrorCode setPantiltLimitDownleft(uint32_t pan_limit, uint32_t tilt_limit);
        ErrorCode setPantiltLimitDownleftClear();
        ErrorCode setPantiltLimitUprightClear();
        ErrorCode setDatascreenOn();
        ErrorCode setDatascreenOff();
        ErrorCode setDatascreenOnoff();
        ErrorCode setSpotAeOn();
        ErrorCode setSpotAeOff();
        ErrorCode setSpotAePosition(uint8_t x_position, uint8_t y_position);
        /* INQUIRIES */
        ErrorCode getPower(uint8_t* power);
        ErrorCode getDzoom(uint8_t* power);
        ErrorCode getDzoomLimit(uint8_t* value);
        ErrorCode getZoomValue(uint16_t* value);
        ErrorCode getFocusAuto(bool* on);
        ErrorCode getFocusValue(uint16_t* value);
        ErrorCode getFocusAutoSense(uint8_t* mode);
        ErrorCode getFocusNearLimit(uint16_t* value);
        ErrorCode getWhitebalMode(uint8_t* mode);
        ErrorCode getRgainValue(uint16_t* value);
        ErrorCode getBgainValue(uint16_t* value);
        ErrorCode getAutoExpMode(uint8_t* mode);
        ErrorCode getSlowShutterAuto(uint8_t* mode);
        ErrorCode getShutterValue(uint16_t* value);
        ErrorCode getIrisValue(uint16_t* value);
        ErrorCode getGainValue(uint16_t* value);
        ErrorCode getBrightValue(uint16_t* value);
        ErrorCode getExpCompPower(uint8_t* power);
        ErrorCode getExpCompValue(uint16_t* value);
        ErrorCode getBacklightComp(uint8_t* power);
        ErrorCode getApertureValue(uint16_t* value);
        ErrorCode getZeroLuxShot(uint8_t* power);
        ErrorCode getIrLed(uint8_t* power);
        ErrorCode getWideMode(uint8_t* mode);
        ErrorCode getMirror(uint8_t* power);
        ErrorCode getFreeze(uint8_t* power);
        ErrorCode getPictureEffect(uint8_t* mode);
        ErrorCode getDigitalEffect(uint8_t* mode);
        ErrorCode getDigitalEffectLevel(uint16_t* value);
        ErrorCode getMemory(uint8_t* channel);
        ErrorCode getDisplay(uint8_t* power);
        ErrorCode getId(uint16_t* id);
        ErrorCode getVideosystem(uint8_t* system);
        ErrorCode getPantiltMode(uint16_t* status);
        ErrorCode getPantiltMaxspeed(uint8_t* max_pan_speed, uint8_t* max_tilt_speed);
        ErrorCode getPantiltPosition(uint16_t* pan_position, uint16_t* tilt_position);
        ErrorCode getDatascreen(uint8_t* status);
        /* SPECIAL FUNCTIONS FOR D30/31 */
        ErrorCode setWideConLens(uint8_t power);
        ErrorCode setAtModeOnoff();
        ErrorCode setAtMode(uint8_t power);
        ErrorCode setAtAeOnoff();
        ErrorCode setAtAe(uint8_t power);
        ErrorCode setAtAutozoomOnoff();
        ErrorCode setAtAutozoom(uint8_t power);
        ErrorCode setAtmdFramedisplayOnoff();
        ErrorCode setAtmdFramedisplay(uint8_t power);
        ErrorCode setAtFrameoffsetOnoff();
        ErrorCode setAtFrameoffset(uint8_t power);
        ErrorCode setAtmdStartstop();
        ErrorCode setAtChase(uint8_t power);
        ErrorCode setAtChaseNext();
        ErrorCode setMdModeOnoff();
        ErrorCode setMdMode(uint8_t power);
        ErrorCode setMdFrame();
        ErrorCode setMdDetect();
        ErrorCode setAtEntry(uint8_t power);
        ErrorCode setAtLostinfo();
        ErrorCode setMdLostinfo();
        ErrorCode setMdAdjustYlevel(uint8_t power);
        ErrorCode setMdAdjustHuelevel(uint8_t power);
        ErrorCode setMdAdjustSize(uint8_t power);
        ErrorCode setMdAdjustDisptime(uint8_t power);
        ErrorCode setMdAdjustRefmode(uint8_t power);
        ErrorCode setMdAdjustReftime(uint8_t power);
        ErrorCode setMdMeasureMode1Onoff();
        ErrorCode setMdMeasureMode1(uint8_t power);
        ErrorCode setMdMeasureMode2Onoff();
        ErrorCode setMdMeasureMode2(uint8_t power);
        ErrorCode getKeylock(uint8_t* power);
        ErrorCode getWideConLens(uint8_t* power);
        ErrorCode getAtmdMode(uint8_t* power);
        ErrorCode getAtMode(uint16_t* value);
        ErrorCode getAtEntry(uint8_t* power);
        ErrorCode getMdMode(uint16_t* value);
        ErrorCode getMdYlevel(uint8_t* power);
        ErrorCode getMdHuelevel(uint8_t* power);
        ErrorCode getMdSize(uint8_t* power);
        ErrorCode getMdDisptime(uint8_t* power);
        ErrorCode getMdRefmode(uint8_t* power);
        ErrorCode getMdReftime(uint8_t* power);
        ErrorCode getAtObjPos(uint8_t* xpos, uint8_t* ypos, uint8_t* status);
        ErrorCode getMdObjPos(uint8_t* xpos, uint8_t* ypos, uint8_t* status);
        ErrorCode setRegister(uint8_t reg_num, uint8_t reg_val);
        ErrorCode getRegister(uint8_t reg_num, uint8_t* reg_val);

    private:
        ViscaInterface iface;
        ViscaCamera camera;
        int32_t address;
        static void appendByte(ViscaPacket* packet, uint8_t byte);
        static void initPacket(ViscaPacket* packet);
        ErrorCode getReply();
        ErrorCode sendPacketWithReply(ViscaPacket* packet);
        ErrorCode write(const ViscaPacket* packet);
        ErrorCode sendPacket(ViscaPacket* packet);
        ErrorCode read();
        ErrorCode unreadBytes(const uint8_t* buffer, uint32_t* buffer_size);
    };
}