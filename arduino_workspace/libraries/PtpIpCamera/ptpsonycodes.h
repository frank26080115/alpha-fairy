#ifndef _PTPSONYCODES_H_
#define _PTPSONYCODES_H_

#include "ptpcodes.h"

enum
{
    SONYALPHA_OPCODE_SDIOConnect            = 0x9201,
    SONYALPHA_OPCODE_SDIOGetExtDeviceInfo   = 0x9202,
    SONYALPHA_OPCODE_SonyGetDevicePropDesc  = 0x9203,
    SONYALPHA_OPCODE_SonyGetDevicePropValue = 0x9204,
    SONYALPHA_OPCODE_SetControlDeviceA      = 0x9205,
    SONYALPHA_OPCODE_GetControlDeviceDesc   = 0x9206,
    SONYALPHA_OPCODE_SetControlDeviceB      = 0x9207,
    SONYALPHA_OPCODE_GetAllDevicePropData   = 0x9209,
};

enum
{
    SONYALPHA_EVENTCODE_ObjectAdded       = 0xC201,
    SONYALPHA_EVENTCODE_ObjectRemoved     = 0xC202,
    SONYALPHA_EVENTCODE_PropertyChanged   = 0xC203,
};

enum
{
    SONYALPHA_PROPCODE_DPCCompensation    = 0xD200,
    SONYALPHA_PROPCODE_DRangeOptimize     = 0xD201,
    SONYALPHA_PROPCODE_SonyImageSize      = 0xD203,
    SONYALPHA_PROPCODE_ShutterSpeed       = 0xD20D,
    SONYALPHA_PROPCODE_Unknown_0E         = 0xD20E,
    SONYALPHA_PROPCODE_ColorTemp          = 0xD20F,
    SONYALPHA_PROPCODE_CCFilter           = 0xD210,
    SONYALPHA_PROPCODE_AspectRatio        = 0xD211,
    SONYALPHA_PROPCODE_FocusFound         = 0xD213,
    SONYALPHA_PROPCODE_ObjectInMemory     = 0xD215,
    SONYALPHA_PROPCODE_ExposeIndex        = 0xD216,
    SONYALPHA_PROPCODE_SonyBatteryLevel   = 0xD218,
    SONYALPHA_PROPCODE_PictureEffect      = 0xD21B,
    SONYALPHA_PROPCODE_ABFilter           = 0xD21C,
    SONYALPHA_PROPCODE_Recording          = 0xD21D,
    SONYALPHA_PROPCODE_ISO                = 0xD21E,
    SONYALPHA_PROPCODE_AutoFocus          = 0xD2C1,
    SONYALPHA_PROPCODE_FocusMode          = PTP_PROPCODE_FocusMode,
    SONYALPHA_PROPCODE_Capture            = 0xD2C2,
    SONYALPHA_PROPCODE_Movie              = 0xD2C8,
    SONYALPHA_PROPCODE_StillImage         = 0xD2C7,
    SONYALPHA_PROPCODE_DriveMode          = PTP_PROPCODE_StillCaptureMode,
    SONYALPHA_PROPCODE_Aperture           = PTP_PROPCODE_Aperture,
    SONYALPHA_PROPCODE_ExpoComp           = PTP_PROPCODE_ExposureBiasCompensation,
    SONYALPHA_PROPCODE_FocusArea          = 0xD22C,
    SONYALPHA_PROPCODE_FocusPointGet      = 0xD232,
    SONYALPHA_PROPCODE_FocusPointSet      = 0xD2DC,
    SONYALPHA_PROPCODE_ManualFocusMode    = 0xD2D2,
    SONYALPHA_PROPCODE_ManualFocusStep    = 0xD2D1,
    SONYALPHA_PROPCODE_ZoomStep           = 0xD2DD,
    SONYALPHA_PROPCODE_FileFormat         = PTP_PROPCODE_CompressionSetting,
    SONYALPHA_PROPCODE_MovieTime          = 0xD261,
    SONYALPHA_PROPCODE_ManualFocusDist    = 0xD24C,
    SONYALPHA_PROPCODE_FocusAssistWindow  = 0xD254,
    SONYALPHA_PROPCODE_MemoryRemaining_Card1 = 0xD249,
    SONYALPHA_PROPCODE_MemoryRemaining_Card2 = 0xD257,
};

enum
{
    SONYALPHA_DRIVEMODE_Single                        = 0x0001,
    SONYALPHA_DRIVEMODE_ContHi                        = 0x0002,
    SONYALPHA_DRIVEMODE_ContHiPlus                    = 0x8010,
    SONYALPHA_DRIVEMODE_ContLo                        = 0x8012,
    SONYALPHA_DRIVEMODE_ContMid                       = 0x8015,
    SONYALPHA_DRIVEMODE_SelfTimer10s                  = 0x8004,
    SONYALPHA_DRIVEMODE_SelfTimer2s                   = 0x8005,
    SONYALPHA_DRIVEMODE_SelfTimer5s                   = 0x8003,
    SONYALPHA_DRIVEMODE_SelfTimer10sContinuous3Images = 0x8008,
    SONYALPHA_DRIVEMODE_SelfTimer10sContinuous5Images = 0x8009,
    SONYALPHA_DRIVEMODE_SelfTimer2sContinuous3Images  = 0x800E,
    SONYALPHA_DRIVEMODE_SelfTimer2sContinuous5Images  = 0x800F,
    SONYALPHA_DRIVEMODE_SelfTimer5sContinuous3Images  = 0x800C,
    SONYALPHA_DRIVEMODE_SelfTimer5sContinuous5Images  = 0x800D,
    SONYALPHA_DRIVEMODE_Continuous                    = 0x8013,
    SONYALPHA_DRIVEMODE_ContinuousSpeedPriority       = 0x8014,
    SONYALPHA_DRIVEMODE_WhiteBalanceBracketLow        = 0x8018,
    SONYALPHA_DRIVEMODE_WhiteBalanceBracketHigh       = 0x8028,
    SONYALPHA_DRIVEMODE_DRangeOptimizerBracketLow     = 0x8019,
    SONYALPHA_DRIVEMODE_DRangoOptimizerBracketHigh    = 0x8029,
    SONYALPHA_DRIVEMODE_ContinuousBracket10EV3Image   = 0x8311,
    SONYALPHA_DRIVEMODE_ContinuousBracket20EV3Image   = 0x8321,
    SONYALPHA_DRIVEMODE_ContinuousBracket30EV3Image   = 0x8331,
    SONYALPHA_DRIVEMODE_ContinuousBracket03EV3Image   = 0x8337,
    SONYALPHA_DRIVEMODE_ContinuousBracket05EV3Image   = 0x8357,
    SONYALPHA_DRIVEMODE_ContinuousBracket07EV3Image   = 0x8377,
    SONYALPHA_DRIVEMODE_ContinuousBracket10EV5Image   = 0x8511,
    SONYALPHA_DRIVEMODE_ContinuousBracket20EV5Image   = 0x8521,
    SONYALPHA_DRIVEMODE_ContinuousBracket30EV5Image   = 0x8531,
    SONYALPHA_DRIVEMODE_ContinuousBracket03EV5Image   = 0x8537,
    SONYALPHA_DRIVEMODE_ContinuousBracket05EV5Image   = 0x8557,
    SONYALPHA_DRIVEMODE_ContinuousBracket07EV5Image   = 0x8577,
    SONYALPHA_DRIVEMODE_ContinuousBracket10EV9Image   = 0x8911,
    SONYALPHA_DRIVEMODE_ContinuousBracket03EV9Image   = 0x8937,
    SONYALPHA_DRIVEMODE_ContinuousBracket05EV9Image   = 0x8957,
    SONYALPHA_DRIVEMODE_ContinuousBracket07EV9Image   = 0x8977,
    SONYALPHA_DRIVEMODE_SingleBracket10EV3Image       = 0x8310,
    SONYALPHA_DRIVEMODE_SingleBracket20EV3Image       = 0x8320,
    SONYALPHA_DRIVEMODE_SingleBracket30EV3Image       = 0x8330,
    SONYALPHA_DRIVEMODE_SingleBracket03EV3Image       = 0x8336,
    SONYALPHA_DRIVEMODE_SingleBracket05EV3Image       = 0x8356,
    SONYALPHA_DRIVEMODE_SingleBracket07EV3Image       = 0x8376,
    SONYALPHA_DRIVEMODE_SingleBracket10EV5Image       = 0x8510,
    SONYALPHA_DRIVEMODE_SingleBracket20EV5Image       = 0x8520,
    SONYALPHA_DRIVEMODE_SingleBracket30EV5Image       = 0x8530,
    SONYALPHA_DRIVEMODE_SingleBracket03EV5Image       = 0x8536,
    SONYALPHA_DRIVEMODE_SingleBracket05EV5Image       = 0x8556,
    SONYALPHA_DRIVEMODE_SingleBracket07EV5Image       = 0x8576,
    SONYALPHA_DRIVEMODE_SingleBracket10EV9Image       = 0x8910,
    SONYALPHA_DRIVEMODE_SingleBracket03EV9Image       = 0x8936,
    SONYALPHA_DRIVEMODE_SingleBracket05EV9Image       = 0x8956,
    SONYALPHA_DRIVEMODE_SingleBracket07EV9Image       = 0x8976,
};
// note: Does NOT match "CrDriveMode" from "CrDeviceProperty" in CrSDK

enum
{
    SONYALPHA_EXPOMODE_IntelligentAuto   = 0x8000,
    SONYALPHA_EXPOMODE_SuperiorAuto      = 0x8001,
    SONYALPHA_EXPOMODE_P                 = 0x0002,
    SONYALPHA_EXPOMODE_A                 = 0x0003,
    SONYALPHA_EXPOMODE_S                 = 0x0004,
    SONYALPHA_EXPOMODE_M                 = 0x0001,
    SONYALPHA_EXPOMODE_MovieP            = 0x8050,
    SONYALPHA_EXPOMODE_MovieA            = 0x8051,
    SONYALPHA_EXPOMODE_MovieS            = 0x8052,
    SONYALPHA_EXPOMODE_MovieM            = 0x8053,
    SONYALPHA_EXPOMODE_Panoramic         = 0x8041,
    SONYALPHA_EXPOMODE_Portrait          = 0x0007,
    SONYALPHA_EXPOMODE_SportsAction      = 0x8011,
    SONYALPHA_EXPOMODE_Macro             = 0x8015,
    SONYALPHA_EXPOMODE_Landscape         = 0x8014,
    SONYALPHA_EXPOMODE_Sunset            = 0x8012,
    SONYALPHA_EXPOMODE_NightScene        = 0x8013,
    SONYALPHA_EXPOMODE_HandheldTwilight  = 0x8016,
    SONYALPHA_EXPOMODE_NightPortrait     = 0x8017,
    SONYALPHA_EXPOMODE_AntiMotionBlur    = 0x8018,
};
// note: potentially matching "CrExposureProgram" from "CrDeviceProperty" in CrSDK

enum
{
    SONYALPHA_FOCUSAREA_WIDE                      = 0x0001,
    SONYALPHA_FOCUSAREA_ZONE                      = 0x0002,
    SONYALPHA_FOCUSAREA_CENTER                    = 0x0003,
    SONYALPHA_FOCUSAREA_MOVEABLE_SMALL            = 0x0101,
    SONYALPHA_FOCUSAREA_MOVEABLE_MEDIUM           = 0x0102,
    SONYALPHA_FOCUSAREA_MOVEABLE_LARGE            = 0x0103,
    SONYALPHA_FOCUSAREA_MOVEABLE_EXPAND           = 0x0104,
    SONYALPHA_FOCUSAREA_TRACKING_WIDE             = 0x0201,
    SONYALPHA_FOCUSAREA_TRACKING_ZONE             = 0x0202,
    SONYALPHA_FOCUSAREA_TRACKING_CENTER           = 0x0203,
    SONYALPHA_FOCUSAREA_TRACKING_MOVEABLE_SMALL   = 0x0204,
    SONYALPHA_FOCUSAREA_TRACKING_MOVEABLE_MEDIUM  = 0x0205,
    SONYALPHA_FOCUSAREA_TRACKING_MOVEABLE_LARGE   = 0x0206,
    SONYALPHA_FOCUSAREA_TRACKING_MOVEABLE_EXPAND  = 0x0207,
};
// note: Does NOT match "CrFocusArea" from "CrDeviceProperty" in CrSDK

enum
{
    SONYALPHA_FILEFORMAT_RAW              = 0x0010,
    SONYALPHA_FILEFORMAT_RAW_JPEG_STD     = 0x0012,
    SONYALPHA_FILEFORMAT_RAW_JPEG_FINE    = 0x0013,
    SONYALPHA_FILEFORMAT_RAW_JPEG_XFINE   = 0x0014,
    SONYALPHA_FILEFORMAT_JPEG_STD         = 0x0002,
    SONYALPHA_FILEFORMAT_JPEG_FINE        = 0x0003,
    SONYALPHA_FILEFORMAT_JPEG_XFINE       = 0x0004,
};
// note: the quality bits seems to be matching "CrJpegQuality" from "CrDeviceProperty" in CrSDK

#define SONYALPHA_FOCUSPOINT_X_MAX 639
#define SONYALPHA_FOCUSPOINT_Y_MAX 480
#define SONYALPHA_FOCUSPOINT_X_MID ((SONYALPHA_FOCUSPOINT_X_MAX + 1) / 2)
#define SONYALPHA_FOCUSPOINT_Y_MID ((SONYALPHA_FOCUSPOINT_Y_MAX + 0) / 2)
// TODO: unsure if this is specific to the a6600 that I'm testing on, maybe a bigger screen means bigger numbers?

enum
{
    SONYALPHA_FOCUSSTEP_FARTHER_SMALL   =  1,
    SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM  =  3,
    SONYALPHA_FOCUSSTEP_FARTHER_LARGE   =  7,
    SONYALPHA_FOCUSSTEP_CLOSER_SMALL    = -1,
    SONYALPHA_FOCUSSTEP_CLOSER_MEDIUM   = -3,
    SONYALPHA_FOCUSSTEP_CLOSER_LARGE    = -7,
    // TODO: experiment with other step sizes
};

enum
{
    SONYALPHA_ZOOM_TELE = -1,
    SONYALPHA_ZOOM_WIDE =  1,
};

enum
{
    SONYALPHA_AFMODE_AFC = 0x8004,
    SONYALPHA_AFMODE_AFS = 0x0002,
    SONYALPHA_AFMODE_AFA = 0x8005,
    SONYALPHA_AFMODE_DMF = 0x8006,
    SONYALPHA_AFMODE_MF  = 0x0001,
};
// note: Does NOT match "CrFocusMode" from "CrDeviceProperty" in CrSDK

enum
{
    SONYALPHA_FOCUSSTATUS_NONE     = 0x01, // idle
    SONYALPHA_FOCUSSTATUS_FOCUSED  = 0x06, // focused, and tracking
    SONYALPHA_FOCUSSTATUS_HUNTING  = 0x05, // still searching
    SONYALPHA_FOCUSSTATUS_FAILED   = 0x03, // AF failed, unable to acquire
    SONYALPHA_FOCUSSTATUS_LOST     = 0x02, // single AF on target, but then shift out of focus (normal for AF-S)
};
// note: Does NOT match "CrFocusIndicator" from "CrDeviceProperty" in CrSDK

#endif
