#pragma once

#include <cstdint>
#include <termios.h>

using error_code = uint32_t;

//TODO: test with/out inline
inline constexpr uint8_t VISCA_COMMAND = 0x01;
inline constexpr uint8_t VISCA_INQUIRY = 0x09;
inline constexpr uint8_t VISCA_TERMINATOR = 0xFF;
inline constexpr uint8_t VISCA_CATEGORY_INTERFACE = 0x00;
inline constexpr uint8_t VISCA_CATEGORY_CAMERA1 = 0x04;
inline constexpr uint8_t VISCA_CATEGORY_PAN_TILTER = 0x06;
inline constexpr uint8_t VISCA_CATEGORY_CAMERA2 = 0x07;

/* Known Vendor IDs */
inline constexpr uint16_t VISCA_VENDOR_SONY = 0x0020;
/* Known Model IDs. The manual can be taken from
 * http://www.sony.net/Products/ISP/docu_soft/index.html
 */
inline constexpr uint16_t VISCA_MODEL_IX47X = 0x0401; /* from FCB-IX47, FCB-IX470 instruction list */
inline constexpr uint16_t VISCA_MODEL_EX47XL = 0x0402; /* from FCB-EX47L, FCB-EX470L instruction list */
inline constexpr uint16_t VISCA_MODEL_IX10 = 0x0404; /* FCB-IX10, FCB-IX10P instruction list */
inline constexpr uint16_t VISCA_MODEL_EX780 = 0x0411; /* from EX780S(P) tech-manual */
inline constexpr uint16_t VISCA_MODEL_EX480A = 0x0412; /* from EX48A/EX480A tech-manual */
inline constexpr uint16_t VISCA_MODEL_EX480AP = 0x0413;
inline constexpr uint16_t VISCA_MODEL_EX48A = 0x0414;
inline constexpr uint16_t VISCA_MODEL_EX48AP = 0x0414;
inline constexpr uint16_t VISCA_MODEL_EX45M = 0x041E;
inline constexpr uint16_t VISCA_MODEL_EX45MCE = 0x041F;
inline constexpr uint16_t VISCA_MODEL_IX47A = 0x0418; /* from IX47A tech-manual */
inline constexpr uint16_t VISCA_MODEL_IX47AP = 0x0419;
inline constexpr uint16_t VISCA_MODEL_IX45A = 0x041A;
inline constexpr uint16_t VISCA_MODEL_IX45AP = 0x041B;
inline constexpr uint16_t VISCA_MODEL_IX10A = 0x041C; /* from IX10A tech-manual */
inline constexpr uint16_t VISCA_MODEL_IX10AP = 0x041D;
inline constexpr uint16_t VISCA_MODEL_EX780B = 0x0420; /* from EX78/EX780 tech-manual */
inline constexpr uint16_t VISCA_MODEL_EX780BP = 0x0421;
inline constexpr uint16_t VISCA_MODEL_EX78B = 0x0422;
inline constexpr uint16_t VISCA_MODEL_EX78BP = 0x0423;
inline constexpr uint16_t VISCA_MODEL_EX480B = 0x0424; /* from EX48/EX480 tech-manual */
inline constexpr uint16_t VISCA_MODEL_EX480BP = 0x0425;
inline constexpr uint16_t VISCA_MODEL_EX48B = 0x0426;
inline constexpr uint16_t VISCA_MODEL_EX48BP = 0x0427;
inline constexpr uint16_t VISCA_MODEL_EX980S = 0x042E; /* from EX98/EX980 tech-manual */
inline constexpr uint16_t VISCA_MODEL_EX980SP = 0x042F;
inline constexpr uint16_t VISCA_MODEL_EX980 = 0x0430;
inline constexpr uint16_t VISCA_MODEL_EX980P = 0x0431;
inline constexpr uint16_t VISCA_MODEL_H10 = 0x044A; /* from H10 tech-manual */
/* Commands/inquiries codes */
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
/***************/
/* ERROR CODES */
/***************/
/* these two are defined by me, not by the specs. */
inline constexpr error_code VISCA_SUCCESS = 0x00;
inline constexpr error_code VISCA_FAILURE = 0xFF;
/* specs errors: */
inline constexpr error_code VISCA_ERROR_MESSAGE_LENGTH = 0x01;
inline constexpr error_code VISCA_ERROR_SYNTAX = 0x02;
inline constexpr error_code VISCA_ERROR_CMD_BUFFER_FULL = 0x03;
inline constexpr error_code VISCA_ERROR_CMD_CANCELLED = 0x04;
inline constexpr error_code VISCA_ERROR_NO_SOCKET = 0x05;
inline constexpr error_code VISCA_ERROR_CMD_NOT_EXECUTABLE = 0x41;
/* Generic definitions */
inline constexpr error_code VISCA_ON = 0x02;
inline constexpr error_code VISCA_OFF = 0x03;
inline constexpr error_code VISCA_RESET = 0x00;
inline constexpr error_code VISCA_UP = 0x02;
inline constexpr error_code VISCA_DOWN = 0x03;
/* response types */
inline constexpr error_code VISCA_RESPONSE_CLEAR = 0x40;
inline constexpr error_code VISCA_RESPONSE_ADDRESS = 0x30;
inline constexpr error_code VISCA_RESPONSE_ACK = 0x40;
inline constexpr error_code VISCA_RESPONSE_COMPLETED = 0x50;
inline constexpr error_code VISCA_RESPONSE_ERROR = 0x60;

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
            struct termios options;
            uint32_t baud;
            // VISCA data:
            uint32_t address;
            uint32_t broadcast;
            // RS232 input buffer
            uint8_t ibuf[VISCA_INPUT_BUFFER_SIZE];
            uint32_t bytes;
            uint32_t type;
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

        static error_code setAddress(ViscaInterface* iface, int* camera_num);
        static error_code clear(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code getCameraInfo(ViscaInterface* iface, ViscaCamera* camera);
        static error_code openSerial(ViscaInterface* iface, const char* device_name);
        static error_code closeSerial(ViscaInterface* iface);
        /* COMMANDS */
        static error_code setPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setKeylock(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setCameraId(ViscaInterface* iface, const ViscaCamera* camera, uint16_t id);
        static error_code setZoomTele(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setZoomWide(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setZoomStop(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setZoomTeleSpeed(ViscaInterface* iface, const ViscaCamera* camera, uint32_t speed);
        static error_code setZoomWideSpeed(ViscaInterface* iface, const ViscaCamera* camera, uint32_t speed);
        static error_code setZoomValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t zoom);
        static error_code setZoomAndFocusValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t zoom,
                                             uint32_t focus);
        static error_code setDzoom(ViscaInterface* iface, const ViscaCamera* camera, uint32_t power);
        static error_code setDzoomLimit(ViscaInterface* iface, const ViscaCamera* camera, uint32_t limit);
        static error_code setDzoomMode(ViscaInterface* iface, const ViscaCamera* camera, uint32_t power);
        static error_code setFocusFar(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setFocusNear(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setFocusStop(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setFocusFarSpeed(ViscaInterface* iface, const ViscaCamera* camera, uint32_t speed);
        static error_code setFocusNearSpeed(ViscaInterface* iface, const ViscaCamera* camera, uint32_t speed);
        static error_code setFocusValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t focus);
        static error_code setFocusAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setFocusOnePush(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setFocusInfinity(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setFocusAutosenseHigh(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setFocusAutosenseLow(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setFocusNearLimit(ViscaInterface* iface, const ViscaCamera* camera, uint32_t limit);
        static error_code setWhitebalMode(ViscaInterface* iface, const ViscaCamera* camera, uint32_t mode);
        static error_code setWhitebalOnePush(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setRgainUp(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setRgainDown(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setRgainReset(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setRgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static error_code setBgainUp(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setBgainDown(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setBgainReset(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setBgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static error_code setShutterUp(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setShutterDown(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setShutterReset(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setShutterValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static error_code setIrisUp(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setIrisDown(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setIrisReset(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setIrisValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static error_code setGainUp(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setGainDown(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setGainReset(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setGainValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static error_code setBrightUp(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setBrightDown(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setBrightReset(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setBrightValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static error_code setApertureUp(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setApertureDown(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setApertureReset(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setApertureValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static error_code setExpCompUp(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setExpCompDown(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setExpCompReset(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setExpCompValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static error_code setExpCompPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setAutoExpMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t mode);
        static error_code setSlowShutterAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setBacklightComp(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setZeroLuxShot(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setIrLed(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setWideMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t mode);
        static error_code setMirror(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setFreeze(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setPictureEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t mode);
        static error_code setDigitalEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t mode);
        static error_code setDigitalEffectLevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t level);
        static error_code setCamStabilizer(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code memorySet(ViscaInterface* iface, const ViscaCamera* camera, uint8_t channel);
        static error_code memoryRecall(ViscaInterface* iface, const ViscaCamera* camera, uint8_t channel);
        static error_code memoryReset(ViscaInterface* iface, const ViscaCamera* camera, uint8_t channel);
        static error_code setDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setDateTime(ViscaInterface* iface, const ViscaCamera* camera, uint32_t year, uint32_t month,
                                    uint32_t day, uint32_t hour, uint32_t minute);
        static error_code setDateDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setTimeDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setTitleDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setTitleClear(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setTitleParams(ViscaInterface* iface, const ViscaCamera* camera, const ViscaTitleData* title);
        static error_code setTitle(ViscaInterface* iface, const ViscaCamera* camera, const ViscaTitleData* title);
        static error_code setIrreceiveOn(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setIrreceiveOff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setIrreceiveOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        /*  pan_speed should be in the range 01 - 18.
            tilt_speed should be in the range 01 - 14 */
        static error_code setPantiltUp(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                     uint32_t tilt_speed);
        static error_code setPantiltDown(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                       uint32_t tilt_speed);
        static error_code setPantiltLeft(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                       uint32_t tilt_speed);
        static error_code setPantiltRight(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                        uint32_t tilt_speed);
        static error_code setPantiltUpleft(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                         uint32_t tilt_speed);
        static error_code setPantiltUpright(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                          uint32_t tilt_speed);
        static error_code setPantiltDownleft(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                           uint32_t tilt_speed);
        static error_code setPantiltDownright(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                            uint32_t tilt_speed);
        static error_code setPantiltStop(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                       uint32_t tilt_speed);
        /*  pan_speed should be in the range 01 - 18.
            tilt_speed should be in the range 01 - 14
            pan_position should be in the range -880 - 880 (0xFC90 - 0x370)
            tilt_position should be in range -300 - 300 (0xFED4 - 0x12C)  */
        static error_code setPantiltAbsolutePosition(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                                   uint32_t tilt_speed, uint32_t pan_position, uint32_t tilt_position);
        static error_code setPantiltRelativePosition(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                                   uint32_t tilt_speed, uint32_t pan_position, uint32_t tilt_position);
        static error_code setPantiltHome(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setPantiltReset(ViscaInterface* iface, const ViscaCamera* camera);
        /*  pan_limit should be in the range -880 - 880 (0xFC90 - 0x370)
            tilt_limit should be in range -300 - 300 (0xFED4 - 0x12C)  */
        static error_code setPantiltLimitUpright(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_limit,
                                               uint32_t tilt_limit);
        static error_code setPantiltLimitDownleft(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_limit,
                                                uint32_t tilt_limit);
        static error_code setPantiltLimitDownleftClear(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setPantiltLimitUprightClear(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setDatascreenOn(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setDatascreenOff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setDatascreenOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setSpotAeOn(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setSpotAeOff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setSpotAePosition(ViscaInterface* iface, const ViscaCamera* camera, uint8_t x_position,
                                          uint8_t y_position);
        /* INQUIRIES */
        static error_code getPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getDzoom(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getDzoomLimit(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* value);
        static error_code getZoomValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getFocusAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getFocusValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getFocusAutoSense(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static error_code getFocusNearLimit(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getWhitebalMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static error_code getRgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getBgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getAutoExpMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static error_code getSlowShutterAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static error_code getShutterValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getIrisValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getGainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getBrightValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getExpCompPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getExpCompValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getBacklightComp(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getApertureValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getZeroLuxShot(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getIrLed(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getWideMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static error_code getMirror(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getFreeze(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getPictureEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static error_code getDigitalEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static error_code getDigitalEffectLevel(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getMemory(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* channel);
        static error_code getDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getId(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* id);
        static error_code getVideosystem(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* system);
        static error_code getPantiltMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* status);
        static error_code getPantiltMaxspeed(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* max_pan_speed,
                                           uint8_t* max_tilt_speed);
        static error_code getPantiltPosition(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* pan_position, uint16_t* tilt_position);
        static error_code getDatascreen(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* status);
        /* SPECIAL FUNCTIONS FOR D30/31 */
        static error_code setWideConLens(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setAtModeOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setAtMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setAtAeOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setAtAe(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setAtAutozoomOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setAtAutozoom(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setAtmdFramedisplayOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setAtmdFramedisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setAtFrameoffsetOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setAtFrameoffset(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setAtmdStartstop(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setAtChase(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setAtChaseNext(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setMdModeOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setMdMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setMdFrame(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setMdDetect(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setAtEntry(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setAtLostinfo(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setMdLostinfo(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setMdAdjustYlevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setMdAdjustHuelevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setMdAdjustSize(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setMdAdjustDisptime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setMdAdjustRefmode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setMdAdjustReftime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setMdMeasureMode1Onoff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setMdMeasureMode1(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code setMdMeasureMode2Onoff(ViscaInterface* iface, const ViscaCamera* camera);
        static error_code setMdMeasureMode2(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static error_code getKeylock(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getWideConLens(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getAtmdMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getAtMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getAtEntry(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getMdMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static error_code getMdYlevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getMdHuelevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getMdSize(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getMdDisptime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getMdRefmode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getMdReftime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static error_code getAtObjPos(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* xpos, uint8_t* ypos,
                                    uint8_t* status);
        static error_code getMdObjPos(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* xpos, uint8_t* ypos,
                                    uint8_t* status);
        static error_code setRegister(ViscaInterface* iface, const ViscaCamera* camera, uint8_t reg_num, uint8_t reg_val);
        static error_code getRegister(ViscaInterface* iface, const ViscaCamera* camera, uint8_t reg_num,
                                    uint8_t* reg_val);

    private:
        static void appendByte(ViscaPacket* packet, uint8_t byte);
        static void initPacket(ViscaPacket* packet);
        static error_code getReply(ViscaInterface* iface);
        static error_code sendPacketWithReply(ViscaInterface* iface, const ViscaCamera* camera, ViscaPacket* packet);
        static error_code writePacketData(const ViscaInterface* iface, const ViscaPacket* packet);
        static error_code sendPacket(const ViscaInterface* iface, const ViscaCamera* camera, ViscaPacket* packet);
        static error_code getPacket(ViscaInterface* iface);
        static error_code unreadBytes(const ViscaInterface* iface, uint8_t* buffer, uint32_t* buffer_size);
    };
}