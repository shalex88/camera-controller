#pragma once

#include <cstdint>
#include <termios.h>

#define VISCA_COMMAND                    0x01
#define VISCA_INQUIRY                    0x09
#define VISCA_TERMINATOR                 0xFF
#define VISCA_CATEGORY_INTERFACE         0x00
#define VISCA_CATEGORY_CAMERA1           0x04
#define VISCA_CATEGORY_PAN_TILTER        0x06
#define VISCA_CATEGORY_CAMERA2           0x07
/* Known Vendor IDs */
#define VISCA_VENDOR_SONY    0x0020
/* Known Model IDs. The manual can be taken from
 * http://www.sony.net/Products/ISP/docu_soft/index.html
 */
#define VISCA_MODEL_IX47X    0x0401          /* from FCB-IX47, FCB-IX470 instruction list */
#define VISCA_MODEL_EX47XL   0x0402          /* from FCB-EX47L, FCB-EX470L instruction list */
#define VISCA_MODEL_IX10     0x0404          /* FCB-IX10, FCB-IX10P instruction list */
#define VISCA_MODEL_EX780    0x0411          /* from EX780S(P) tech-manual */
#define VISCA_MODEL_EX480A   0x0412          /* from EX48A/EX480A tech-manual */
#define VISCA_MODEL_EX480AP  0x0413
#define VISCA_MODEL_EX48A    0x0414
#define VISCA_MODEL_EX48AP   0x0414
#define VISCA_MODEL_EX45M    0x041E
#define VISCA_MODEL_EX45MCE  0x041F
#define VISCA_MODEL_IX47A    0x0418          /* from IX47A tech-manual */
#define VISCA_MODEL_IX47AP   0x0419
#define VISCA_MODEL_IX45A    0x041A
#define VISCA_MODEL_IX45AP   0x041B
#define VISCA_MODEL_IX10A    0x041C          /* from IX10A tech-manual */
#define VISCA_MODEL_IX10AP   0x041D
#define VISCA_MODEL_EX780B   0x0420          /* from EX78/EX780 tech-manual */
#define VISCA_MODEL_EX780BP  0x0421
#define VISCA_MODEL_EX78B    0x0422
#define VISCA_MODEL_EX78BP   0x0423
#define VISCA_MODEL_EX480B   0x0424          /* from EX48/EX480 tech-manual */
#define VISCA_MODEL_EX480BP  0x0425
#define VISCA_MODEL_EX48B    0x0426
#define VISCA_MODEL_EX48BP   0x0427
#define VISCA_MODEL_EX980S   0x042E          /* from EX98/EX980 tech-manual */
#define VISCA_MODEL_EX980SP  0x042F
#define VISCA_MODEL_EX980    0x0430
#define VISCA_MODEL_EX980P   0x0431
#define VISCA_MODEL_H10      0x044A      /* from H10 tech-manual */
/* Commands/inquiries codes */
#define VISCA_POWER                      0x00
#define VISCA_DEVICE_INFO                0x02
#define VISCA_KEYLOCK                    0x17
#define VISCA_ID                         0x22
#define VISCA_ZOOM                       0x07
#define   VISCA_ZOOM_STOP                  0x00
#define   VISCA_ZOOM_TELE                  0x02
#define   VISCA_ZOOM_WIDE                  0x03
#define   VISCA_ZOOM_TELE_SPEED            0x20
#define   VISCA_ZOOM_WIDE_SPEED            0x30
#define VISCA_ZOOM_VALUE                 0x47
#define VISCA_ZOOM_FOCUS_VALUE           0x47
#define VISCA_DZOOM                      0x06
#define   VISCA_DZOOM_OFF                  0x03
#define   VISCA_DZOOM_ON                   0x02
#define VISCA_DZOOM_LIMIT                0x26 /* implemented for H10 */
#define   VISCA_DZOOM_1X                   0x00
#define   VISCA_DZOOM_1_5X                 0x01
#define   VISCA_DZOOM_2X                   0x02
#define   VISCA_DZOOM_4X                   0x03
#define   VISCA_DZOOM_8X                   0x04
#define   VISCA_DZOOM_12X                  0x05
#define VISCA_DZOOM_MODE                 0x36
#define   VISCA_DZOOM_COMBINE              0x00
#define   VISCA_DZOOM_SEPARATE             0x01
#define VISCA_FOCUS                      0x08
#define   VISCA_FOCUS_STOP                 0x00
#define   VISCA_FOCUS_FAR                  0x02
#define   VISCA_FOCUS_NEAR                 0x03
#define   VISCA_FOCUS_FAR_SPEED            0x20
#define   VISCA_FOCUS_NEAR_SPEED           0x30
#define VISCA_FOCUS_VALUE                0x48
#define VISCA_FOCUS_AUTO                 0x38
#define   VISCA_FOCUS_AUTO_ON              0x02
#define   VISCA_FOCUS_AUTO_OFF             0x03
#define   VISCA_FOCUS_AUTO_MAN             0x10
#define VISCA_FOCUS_ONE_PUSH             0x18
#define   VISCA_FOCUS_ONE_PUSH_TRIG        0x01
#define   VISCA_FOCUS_ONE_PUSH_INF         0x02
#define VISCA_FOCUS_AUTO_SENSE           0x58
#define   VISCA_FOCUS_AUTO_SENSE_HIGH      0x02
#define   VISCA_FOCUS_AUTO_SENSE_LOW       0x03
#define VISCA_FOCUS_NEAR_LIMIT           0x28
#define VISCA_WB                         0x35
#define   VISCA_WB_AUTO                    0x00
#define   VISCA_WB_INDOOR                  0x01
#define   VISCA_WB_OUTDOOR                 0x02
#define   VISCA_WB_ONE_PUSH                0x03
#define   VISCA_WB_ATW                     0x04
#define   VISCA_WB_MANUAL                  0x05
#define VISCA_WB_TRIGGER                 0x10
#define   VISCA_WB_ONE_PUSH_TRIG           0x05
#define VISCA_RGAIN                      0x03
#define VISCA_RGAIN_VALUE                0x43
#define VISCA_BGAIN                      0x04
#define VISCA_BGAIN_VALUE                0x44
#define VISCA_AUTO_EXP                   0x39
#define   VISCA_AUTO_EXP_FULL_AUTO         0x00
#define   VISCA_AUTO_EXP_MANUAL            0x03
#define   VISCA_AUTO_EXP_SHUTTER_PRIORITY  0x0A
#define   VISCA_AUTO_EXP_IRIS_PRIORITY     0x0B
#define   VISCA_AUTO_EXP_GAIN_PRIORITY     0x0C
#define   VISCA_AUTO_EXP_BRIGHT            0x0D
#define   VISCA_AUTO_EXP_SHUTTER_AUTO      0x1A
#define   VISCA_AUTO_EXP_IRIS_AUTO         0x1B
#define   VISCA_AUTO_EXP_GAIN_AUTO         0x1C
#define VISCA_SLOW_SHUTTER               0x5A
#define   VISCA_SLOW_SHUTTER_AUTO          0x02
#define   VISCA_SLOW_SHUTTER_MANUAL        0x03
#define VISCA_SHUTTER                    0x0A
#define VISCA_SHUTTER_VALUE              0x4A
#define VISCA_IRIS                       0x0B
#define VISCA_IRIS_VALUE                 0x4B
#define VISCA_GAIN                       0x0C
#define VISCA_GAIN_VALUE                 0x4C
#define VISCA_BRIGHT                     0x0D
#define VISCA_BRIGHT_VALUE               0x4D
#define VISCA_EXP_COMP                   0x0E
#define VISCA_EXP_COMP_POWER             0x3E
#define VISCA_EXP_COMP_VALUE             0x4E
#define VISCA_BACKLIGHT_COMP             0x33
#define VISCA_SPOT_AE                    0x59
#define   VISCA_SPOT_AE_ON               0x02
#define   VISCA_SPOT_AE_OFF              0x03
#define VISCA_SPOT_AE_POSITION           0x29
#define VISCA_APERTURE                   0x02
#define VISCA_APERTURE_VALUE             0x42
#define VISCA_ZERO_LUX                   0x01
#define VISCA_IR_LED                     0x31
#define VISCA_WIDE_MODE                  0x60
#define   VISCA_WIDE_MODE_OFF              0x00
#define   VISCA_WIDE_MODE_CINEMA           0x01
#define   VISCA_WIDE_MODE_16_9             0x02
#define VISCA_MIRROR                     0x61
#define VISCA_FREEZE                     0x62
#define   VISCA_FREEZE_ON                  0x02
#define   VISCA_FREEZE_OFF                 0x03
#define VISCA_PICTURE_EFFECT             0x63
#define   VISCA_PICTURE_EFFECT_OFF         0x00
#define   VISCA_PICTURE_EFFECT_PASTEL      0x01
#define   VISCA_PICTURE_EFFECT_NEGATIVE    0x02
#define   VISCA_PICTURE_EFFECT_SEPIA       0x03
#define   VISCA_PICTURE_EFFECT_BW          0x04
#define   VISCA_PICTURE_EFFECT_SOLARIZE    0x05
#define   VISCA_PICTURE_EFFECT_MOSAIC      0x06
#define   VISCA_PICTURE_EFFECT_SLIM        0x07
#define   VISCA_PICTURE_EFFECT_STRETCH     0x08
#define VISCA_DIGITAL_EFFECT             0x64
#define   VISCA_DIGITAL_EFFECT_OFF         0x00
#define   VISCA_DIGITAL_EFFECT_STILL       0x01
#define   VISCA_DIGITAL_EFFECT_FLASH       0x02
#define   VISCA_DIGITAL_EFFECT_LUMI        0x03
#define   VISCA_DIGITAL_EFFECT_TRAIL       0x04
#define VISCA_DIGITAL_EFFECT_LEVEL       0x65
#define VISCA_CAM_STABILIZER             0x34
#define   VISCA_CAM_STABILIZER_ON         0x02
#define   VISCA_CAM_STABILIZER_OFF        0x03
#define VISCA_MEMORY                     0x3F
#define   VISCA_MEMORY_RESET               0x00
#define   VISCA_MEMORY_SET                 0x01
#define   VISCA_MEMORY_RECALL              0x02
#define     VISCA_MEMORY_0                   0x00
#define     VISCA_MEMORY_1                   0x01
#define     VISCA_MEMORY_2                   0x02
#define     VISCA_MEMORY_3                   0x03
#define     VISCA_MEMORY_4                   0x04
#define     VISCA_MEMORY_5                   0x05
#define     VISCA_MEMORY_CUSTOM              0x7F
#define VISCA_DISPLAY                    0x15
#define   VISCA_DISPLAY_ON                  0x02
#define   VISCA_DISPLAY_OFF                 0x03
#define   VISCA_DISPLAY_TOGGLE              0x10
#define VISCA_DATE_TIME_SET              0x70
#define VISCA_DATE_DISPLAY               0x71
#define VISCA_TIME_DISPLAY               0x72
#define VISCA_TITLE_DISPLAY              0x74
#define   VISCA_TITLE_DISPLAY_CLEAR        0x00
#define   VISCA_TITLE_DISPLAY_ON           0x02
#define   VISCA_TITLE_DISPLAY_OFF          0x03
#define VISCA_TITLE_SET                  0x73
#define   VISCA_TITLE_SET_PARAMS           0x00
#define   VISCA_TITLE_SET_PART1            0x01
#define   VISCA_TITLE_SET_PART2            0x02
#define VISCA_IRRECEIVE                   0x08
#define   VISCA_IRRECEIVE_ON              0x02
#define   VISCA_IRRECEIVE_OFF             0x03
#define   VISCA_IRRECEIVE_ONOFF           0x10
#define VISCA_PT_DRIVE                     0x01
#define   VISCA_PT_DRIVE_HORIZ_LEFT        0x01
#define   VISCA_PT_DRIVE_HORIZ_RIGHT       0x02
#define   VISCA_PT_DRIVE_HORIZ_STOP        0x03
#define   VISCA_PT_DRIVE_VERT_UP           0x01
#define   VISCA_PT_DRIVE_VERT_DOWN         0x02
#define   VISCA_PT_DRIVE_VERT_STOP         0x03
#define VISCA_PT_ABSOLUTE_POSITION         0x02
#define VISCA_PT_RELATIVE_POSITION         0x03
#define VISCA_PT_HOME                      0x04
#define VISCA_PT_RESET                     0x05
#define VISCA_PT_LIMITSET                  0x07
#define   VISCA_PT_LIMITSET_SET            0x00
#define   VISCA_PT_LIMITSET_CLEAR          0x01
#define     VISCA_PT_LIMITSET_SET_UR       0x01
#define     VISCA_PT_LIMITSET_SET_DL       0x00
#define VISCA_PT_DATASCREEN                0x06
#define   VISCA_PT_DATASCREEN_ON           0x02
#define   VISCA_PT_DATASCREEN_OFF          0x03
#define   VISCA_PT_DATASCREEN_ONOFF        0x10
#define VISCA_PT_VIDEOSYSTEM_INQ           0x23
#define VISCA_PT_MODE_INQ                  0x10
#define VISCA_PT_MAXSPEED_INQ              0x11
#define VISCA_PT_POSITION_INQ              0x12
#define VISCA_PT_DATASCREEN_INQ            0x06
/**************************/
/* DIRECT REGISTER ACCESS */
/**************************/
#define VISCA_REGISTER_VALUE              0x24
#define VISCA_REGISTER_VISCA_BAUD          0x00
#define VISCA_REGISTER_BD9600               0x00
#define VISCA_REGISTER_BD19200              0x01
#define VISCA_REGISTER_BD38400              0x02
/* FCB-H10: Video Standard */
#define VISCA_REGISTER_VIDEO_SIGNAL        0x70
#define VISCA_REGISTER_VIDEO_1080I_60       0x01
#define VISCA_REGISTER_VIDEO_720P_60        0x02
#define VISCA_REGISTER_VIDEO_D1_CROP_60     0x03
#define VISCA_REGISTER_VIDEO_D1_SQ_60       0x04
#define VISCA_REGISTER_VIDEO_1080I_50       0x11
#define VISCA_REGISTER_VIDEO_720P_50        0x12
#define VISCA_REGISTER_VIDEO_D1_CROP_50     0x13
#define VISCA_REGISTER_VIDEO_D1_SQ_50       0x14
/*****************/
/* D30/D31 CODES */
/*****************/
#define VISCA_WIDE_CON_LENS        0x26
#define   VISCA_WIDE_CON_LENS_SET          0x00
#define VISCA_AT_MODE                      0x01
#define   VISCA_AT_ONOFF                   0x10
#define VISCA_AT_AE                        0x02
#define VISCA_AT_AUTOZOOM                  0x03
#define VISCA_ATMD_FRAMEDISPLAY            0x04
#define VISCA_AT_FRAMEOFFSET               0x05
#define VISCA_ATMD_STARTSTOP               0x06
#define VISCA_AT_CHASE                     0x07
#define   VISCA_AT_CHASE_NEXT              0x10
#define VISCA_MD_MODE                      0x08
#define   VISCA_MD_ONOFF                   0x10
#define VISCA_MD_FRAME                     0x09
#define VISCA_MD_DETECT                    0x0A
#define VISCA_MD_ADJUST                    0x00
#define   VISCA_MD_ADJUST_YLEVEL           0x0B
#define   VISCA_MD_ADJUST_HUELEVEL         0x0C
#define   VISCA_MD_ADJUST_SIZE             0x0D
#define   VISCA_MD_ADJUST_DISPTIME         0x0F
#define   VISCA_MD_ADJUST_REFTIME          0x0B
#define   VISCA_MD_ADJUST_REFMODE          0x10
#define VISCA_AT_ENTRY                     0x15
#define VISCA_AT_LOSTINFO                  0x20
#define VISCA_MD_LOSTINFO                  0x21
#define VISCA_ATMD_LOSTINFO1               0x20
#define VISCA_ATMD_LOSTINFO2               0x07
#define VISCA_MD_MEASURE_MODE_1            0x27
#define VISCA_MD_MEASURE_MODE_2            0x28
#define VISCA_ATMD_MODE                    0x22
#define VISCA_AT_MODE_QUERY                0x23
#define VISCA_MD_MODE_QUERY                0x24
#define VISCA_MD_REFTIME_QUERY             0x11
#define VISCA_AT_POSITION                  0x20
#define VISCA_MD_POSITION                  0x21
/***************/
/* ERROR CODES */
/***************/
/* these two are defined by me, not by the specs. */
#define VISCA_SUCCESS                    0x00
#define VISCA_FAILURE                    0xFF
/* specs errors: */
#define VISCA_ERROR_MESSAGE_LENGTH       0x01
#define VISCA_ERROR_SYNTAX               0x02
#define VISCA_ERROR_CMD_BUFFER_FULL      0x03
#define VISCA_ERROR_CMD_CANCELLED        0x04
#define VISCA_ERROR_NO_SOCKET            0x05
#define VISCA_ERROR_CMD_NOT_EXECUTABLE   0x41
/* Generic definitions */
#define VISCA_ON                         0x02
#define VISCA_OFF                        0x03
#define VISCA_RESET                      0x00
#define VISCA_UP                         0x02
#define VISCA_DOWN                       0x03
/* response types */
#define VISCA_RESPONSE_CLEAR             0x40
#define VISCA_RESPONSE_ADDRESS           0x30
#define VISCA_RESPONSE_ACK               0x40
#define VISCA_RESPONSE_COMPLETED         0x50
#define VISCA_RESPONSE_ERROR             0x60
/* timeout in us */
#define VISCA_SERIAL_WAIT              100000
/* size of the local packet buffer */
#define VISCA_INPUT_BUFFER_SIZE          1024

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
            unsigned char ibuf[VISCA_INPUT_BUFFER_SIZE];
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
            unsigned char title[20];
        };

        struct ViscaPacket {
            unsigned char bytes[32];
            uint32_t length;
        };

        Visca() = default;
        ~Visca() = default;

        static uint32_t setAddress(ViscaInterface* iface, int* camera_num);
        static uint32_t clear(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t getCameraInfo(ViscaInterface* iface, ViscaCamera* camera);
        static uint32_t openSerial(ViscaInterface* iface, const char* device_name);
        static uint32_t closeSerial(ViscaInterface* iface);
        /* COMMANDS */
        static uint32_t setPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setKeylock(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setCameraId(ViscaInterface* iface, const ViscaCamera* camera, uint16_t id);
        static uint32_t setZoomTele(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setZoomWide(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setZoomStop(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setZoomTeleSpeed(ViscaInterface* iface, const ViscaCamera* camera, uint32_t speed);
        static uint32_t setZoomWideSpeed(ViscaInterface* iface, const ViscaCamera* camera, uint32_t speed);
        static uint32_t setZoomValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t zoom);
        static uint32_t setZoomAndFocusValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t zoom,
                                             uint32_t focus);
        static uint32_t setDzoom(ViscaInterface* iface, const ViscaCamera* camera, uint32_t power);
        static uint32_t setDzoomLimit(ViscaInterface* iface, const ViscaCamera* camera, uint32_t limit);
        static uint32_t setDzoomMode(ViscaInterface* iface, const ViscaCamera* camera, uint32_t power);
        static uint32_t setFocusFar(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setFocusNear(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setFocusStop(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setFocusFarSpeed(ViscaInterface* iface, const ViscaCamera* camera, uint32_t speed);
        static uint32_t setFocusNearSpeed(ViscaInterface* iface, const ViscaCamera* camera, uint32_t speed);
        static uint32_t setFocusValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t focus);
        static uint32_t setFocusAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setFocusOnePush(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setFocusInfinity(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setFocusAutosenseHigh(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setFocusAutosenseLow(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setFocusNearLimit(ViscaInterface* iface, const ViscaCamera* camera, uint32_t limit);
        static uint32_t setWhitebalMode(ViscaInterface* iface, const ViscaCamera* camera, uint32_t mode);
        static uint32_t setWhitebalOnePush(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setRgainUp(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setRgainDown(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setRgainReset(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setRgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static uint32_t setBgainUp(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setBgainDown(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setBgainReset(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setBgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static uint32_t setShutterUp(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setShutterDown(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setShutterReset(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setShutterValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static uint32_t setIrisUp(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setIrisDown(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setIrisReset(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setIrisValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static uint32_t setGainUp(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setGainDown(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setGainReset(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setGainValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static uint32_t setBrightUp(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setBrightDown(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setBrightReset(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setBrightValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static uint32_t setApertureUp(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setApertureDown(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setApertureReset(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setApertureValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static uint32_t setExpCompUp(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setExpCompDown(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setExpCompReset(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setExpCompValue(ViscaInterface* iface, const ViscaCamera* camera, uint32_t value);
        static uint32_t setExpCompPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setAutoExpMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t mode);
        static uint32_t setSlowShutterAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setBacklightComp(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setZeroLuxShot(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setIrLed(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setWideMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t mode);
        static uint32_t setMirror(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setFreeze(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setPictureEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t mode);
        static uint32_t setDigitalEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t mode);
        static uint32_t setDigitalEffectLevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t level);
        static uint32_t setCamStabilizer(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t memorySet(ViscaInterface* iface, const ViscaCamera* camera, uint8_t channel);
        static uint32_t memoryRecall(ViscaInterface* iface, const ViscaCamera* camera, uint8_t channel);
        static uint32_t memoryReset(ViscaInterface* iface, const ViscaCamera* camera, uint8_t channel);
        static uint32_t setDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setDateTime(ViscaInterface* iface, const ViscaCamera* camera, uint32_t year, uint32_t month,
                                    uint32_t day, uint32_t hour, uint32_t minute);
        static uint32_t setDateDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setTimeDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setTitleDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setTitleClear(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setTitleParams(ViscaInterface* iface, const ViscaCamera* camera, const ViscaTitleData* title);
        static uint32_t setTitle(ViscaInterface* iface, const ViscaCamera* camera, const ViscaTitleData* title);
        static uint32_t setIrreceiveOn(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setIrreceiveOff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setIrreceiveOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        /*  pan_speed should be in the range 01 - 18.
            tilt_speed should be in the range 01 - 14 */
        static uint32_t setPantiltUp(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                     uint32_t tilt_speed);
        static uint32_t setPantiltDown(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                       uint32_t tilt_speed);
        static uint32_t setPantiltLeft(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                       uint32_t tilt_speed);
        static uint32_t setPantiltRight(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                        uint32_t tilt_speed);
        static uint32_t setPantiltUpleft(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                         uint32_t tilt_speed);
        static uint32_t setPantiltUpright(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                          uint32_t tilt_speed);
        static uint32_t setPantiltDownleft(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                           uint32_t tilt_speed);
        static uint32_t setPantiltDownright(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                            uint32_t tilt_speed);
        static uint32_t setPantiltStop(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                       uint32_t tilt_speed);
        /*  pan_speed should be in the range 01 - 18.
            tilt_speed should be in the range 01 - 14
            pan_position should be in the range -880 - 880 (0xFC90 - 0x370)
            tilt_position should be in range -300 - 300 (0xFED4 - 0x12C)  */
        static uint32_t setPantiltAbsolutePosition(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                                   uint32_t tilt_speed, uint32_t pan_position, uint32_t tilt_position);
        static uint32_t setPantiltRelativePosition(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_speed,
                                                   uint32_t tilt_speed, uint32_t pan_position, uint32_t tilt_position);
        static uint32_t setPantiltHome(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setPantiltReset(ViscaInterface* iface, const ViscaCamera* camera);
        /*  pan_limit should be in the range -880 - 880 (0xFC90 - 0x370)
            tilt_limit should be in range -300 - 300 (0xFED4 - 0x12C)  */
        static uint32_t setPantiltLimitUpright(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_limit,
                                               uint32_t tilt_limit);
        static uint32_t setPantiltLimitDownleft(ViscaInterface* iface, const ViscaCamera* camera, uint32_t pan_limit,
                                                uint32_t tilt_limit);
        static uint32_t setPantiltLimitDownleftClear(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setPantiltLimitUprightClear(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setDatascreenOn(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setDatascreenOff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setDatascreenOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setSpotAeOn(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setSpotAeOff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setSpotAePosition(ViscaInterface* iface, const ViscaCamera* camera, uint8_t x_position,
                                          uint8_t y_position);
        /* INQUIRIES */
        static uint32_t getPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getDzoom(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getDzoomLimit(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* value);
        static uint32_t getZoomValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getFocusAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getFocusValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getFocusAutoSense(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static uint32_t getFocusNearLimit(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getWhitebalMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static uint32_t getRgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getBgainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getAutoExpMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static uint32_t getSlowShutterAuto(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static uint32_t getShutterValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getIrisValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getGainValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getBrightValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getExpCompPower(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getExpCompValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getBacklightComp(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getApertureValue(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getZeroLuxShot(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getIrLed(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getWideMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static uint32_t getMirror(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getFreeze(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getPictureEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static uint32_t getDigitalEffect(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* mode);
        static uint32_t getDigitalEffectLevel(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getMemory(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* channel);
        static uint32_t getDisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getId(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* id);
        static uint32_t getVideosystem(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* system);
        static uint32_t getPantiltMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* status);
        static uint32_t getPantiltMaxspeed(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* max_pan_speed,
                                           uint8_t* max_tilt_speed);
        static uint32_t getPantiltPosition(ViscaInterface* iface, const ViscaCamera* camera, int16_t* pan_position,
                                           int16_t* tilt_position);
        static uint32_t getDatascreen(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* status);
        /* SPECIAL FUNCTIONS FOR D30/31 */
        static uint32_t setWideConLens(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setAtModeOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setAtMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setAtAeOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setAtAe(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setAtAutozoomOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setAtAutozoom(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setAtmdFramedisplayOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setAtmdFramedisplay(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setAtFrameoffsetOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setAtFrameoffset(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setAtmdStartstop(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setAtChase(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setAtChaseNext(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setMdModeOnoff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setMdMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setMdFrame(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setMdDetect(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setAtEntry(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setAtLostinfo(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setMdLostinfo(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setMdAdjustYlevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setMdAdjustHuelevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setMdAdjustSize(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setMdAdjustDisptime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setMdAdjustRefmode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setMdAdjustReftime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setMdMeasureMode1Onoff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setMdMeasureMode1(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t setMdMeasureMode2Onoff(ViscaInterface* iface, const ViscaCamera* camera);
        static uint32_t setMdMeasureMode2(ViscaInterface* iface, const ViscaCamera* camera, uint8_t power);
        static uint32_t getKeylock(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getWideConLens(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getAtmdMode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getAtMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getAtEntry(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getMdMode(ViscaInterface* iface, const ViscaCamera* camera, uint16_t* value);
        static uint32_t getMdYlevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getMdHuelevel(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getMdSize(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getMdDisptime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getMdRefmode(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getMdReftime(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* power);
        static uint32_t getAtObjPos(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* xpos, uint8_t* ypos,
                                    uint8_t* status);
        static uint32_t getMdObjPos(ViscaInterface* iface, const ViscaCamera* camera, uint8_t* xpos, uint8_t* ypos,
                                    uint8_t* status);
        static uint32_t setRegister(ViscaInterface* iface, const ViscaCamera* camera, uint8_t reg_num, uint8_t reg_val);
        static uint32_t getRegister(ViscaInterface* iface, const ViscaCamera* camera, uint8_t reg_num,
                                    uint8_t* reg_val);

    private:
        static void appendByte(ViscaPacket* packet, unsigned char byte);
        static void initPacket(ViscaPacket* packet);
        static uint32_t getReply(ViscaInterface* iface);
        static uint32_t sendPacketWithReply(ViscaInterface* iface, const ViscaCamera* camera, ViscaPacket* packet);
        static uint32_t writePacketData(const ViscaInterface* iface, const ViscaPacket* packet);
        static uint32_t sendPacket(const ViscaInterface* iface, const ViscaCamera* camera, ViscaPacket* packet);
        static uint32_t getPacket(ViscaInterface* iface);
        static uint32_t unreadBytes(const ViscaInterface* iface, unsigned char* buffer, uint32_t* buffer_size);
    };
}