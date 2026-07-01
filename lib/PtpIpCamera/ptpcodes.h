#ifndef _PTPCODES_H_
#define _PTPCODES_H_

enum
{
    PTP_PKTTYPE_INITCMDREQ                  = 0x0001,
    PTP_PKTTYPE_INITCMDACK                  = 0x0002,
    PTP_PKTTYPE_INITEVENTREQ                = 0x0003,
    PTP_PKTTYPE_INITEVENTACK                = 0x0004,
    PTP_PKTTYPE_INITFAILED                  = 0x0005,
    PTP_PKTTYPE_OPERREQ                     = 0x0006,
    PTP_PKTTYPE_OPERRESP                    = 0x0007,
    PTP_PKTTYPE_EVENT                       = 0x0008,
    PTP_PKTTYPE_STARTDATA                   = 0x0009,
    PTP_PKTTYPE_DATA                        = 0x000A,
    PTP_PKTTYPE_CANCELDATA                  = 0x000B,
    PTP_PKTTYPE_ENDDATA                     = 0x000C,
    PTP_PKTTYPE_PROBEREQ                    = 0x000D,
    PTP_PKTTYPE_PROBERESP                   = 0x000E,
};

enum
{
    PTP_OPCODE_Undefined                    = 0x1000,
    PTP_OPCODE_GetDeviceInfo                = 0x1001,
    PTP_OPCODE_OpenSession                  = 0x1002,
    PTP_OPCODE_CloseSession                 = 0x1003,
    PTP_OPCODE_GetStorageIDs                = 0x1004,
    PTP_OPCODE_GetStorageInfo               = 0x1005,
    PTP_OPCODE_GetNumObjects                = 0x1006,
    PTP_OPCODE_GetObjectHandles             = 0x1007,
    PTP_OPCODE_GetObjectInfo                = 0x1008,
    PTP_OPCODE_GetObject                    = 0x1009,
    PTP_OPCODE_GetThumb                     = 0x100A,
    PTP_OPCODE_DeleteObject                 = 0x100B,
    PTP_OPCODE_SendObjectInfo               = 0x100C,
    PTP_OPCODE_SendObject                   = 0x100D,
    PTP_OPCODE_InitiateCapture              = 0x100E,
    PTP_OPCODE_FormatStore                  = 0x100F,
    PTP_OPCODE_ResetDevice                  = 0x1010,
    PTP_OPCODE_SelfTest                     = 0x1011,
    PTP_OPCODE_SetObjectProtection          = 0x1012,
    PTP_OPCODE_PowerDown                    = 0x1013,
    PTP_OPCODE_GetDevicePropDesc            = 0x1014,
    PTP_OPCODE_GetDevicePropValue           = 0x1015,
    PTP_OPCODE_SetDevicePropValue           = 0x1016,
    PTP_OPCODE_ResetDevicePropValue         = 0x1017,
    PTP_OPCODE_TerminateOpenCapture         = 0x1018,
    PTP_OPCODE_MoveObject                   = 0x1019,
    PTP_OPCODE_CopyObject                   = 0x101A,
    PTP_OPCODE_GetPartialObject             = 0x101B,
    PTP_OPCODE_InitiateOpenCapture          = 0x101C,
    PTP_OPCODE_GetObjectPropValue           = 0x9803,
};

enum
{
    PTP_CONTAINER_UNDEFINED                 = 0x0000,
    PTP_CONTAINER_CMD                       = 0x0001,
    PTP_CONTAINER_DATA                      = 0x0002,
    PTP_CONTAINER_RESPONSE                  = 0x0003,
    PTP_CONTAINER_EVENT                     = 0x0004,
};

enum
{
    PTP_PROPCODE_BatteryLevel               = 0x5001,
    PTP_PROPCODE_FunctionalMode             = 0x5002,
    PTP_PROPCODE_ImageSize                  = 0x5003,
    PTP_PROPCODE_CompressionSetting         = 0x5004,
    PTP_PROPCODE_WhiteBalance               = 0x5005,
    PTP_PROPCODE_RGBGain                    = 0x5006,
    PTP_PROPCODE_Aperture                   = 0x5007,
    PTP_PROPCODE_FocalLength                = 0x5008,
    PTP_PROPCODE_FocusDistance              = 0x5009,
    PTP_PROPCODE_FocusMode                  = 0x500A,
    PTP_PROPCODE_ExposureMeteringMode       = 0x500B,
    PTP_PROPCODE_FlashMode                  = 0x500C,
    PTP_PROPCODE_ExposureTime               = 0x500D,
    PTP_PROPCODE_ExposureProgramMode        = 0x500E,
    PTP_PROPCODE_ExposureIndex              = 0x500F,
    PTP_PROPCODE_ExposureBiasCompensation   = 0x5010,
    PTP_PROPCODE_DateTime                   = 0x5011,
    PTP_PROPCODE_CaptureDelay               = 0x5012,
    PTP_PROPCODE_StillCaptureMode           = 0x5013,
    PTP_PROPCODE_Contrast                   = 0x5014,
    PTP_PROPCODE_Sharpness                  = 0x5015,
    PTP_PROPCODE_DigitalZoom                = 0x5016,
    PTP_PROPCODE_EffectMode                 = 0x5017,
    PTP_PROPCODE_BurstNumber                = 0x5018,
    PTP_PROPCODE_BurstInterval              = 0x5019,
    PTP_PROPCODE_TimelapseNumber            = 0x501A,
    PTP_PROPCODE_TimelapseInterval          = 0x501B,
    PTP_PROPCODE_FocusMeteringMode          = 0x501C,
    PTP_PROPCODE_UploadURL                  = 0x501D,
    PTP_PROPCODE_Artist                     = 0x501E,
    PTP_PROPCODE_CopyrightInfo              = 0x501F,
};

enum
{
    PTP_RESPCODE_Undefined                                  = 0x2000,
    PTP_RESPCODE_OK                                         = 0x2001,
    PTP_RESPCODE_GeneralError                               = 0x2002,
    PTP_RESPCODE_SessionNotOpen                             = 0x2003,
    PTP_RESPCODE_InvalidTransactionID                       = 0x2004,
    PTP_RESPCODE_OperationNotSupported                      = 0x2005,
    PTP_RESPCODE_ParameterNotSupported                      = 0x2006,
    PTP_RESPCODE_IncompleteTransfer                         = 0x2007,
    PTP_RESPCODE_InvalidStorageID                           = 0x2008,
    PTP_RESPCODE_InvalidObjectHandle                        = 0x2009,
    PTP_RESPCODE_DevicePropNotSupported                     = 0x200A,
    PTP_RESPCODE_InvalidObjectFormatCode                    = 0x200B,
    PTP_RESPCODE_StoreFull                                  = 0x200C,
    PTP_RESPCODE_ObjectWriteProtected                       = 0x200D,
    PTP_RESPCODE_StoreReadOnly                              = 0x200E,
    PTP_RESPCODE_AccessDenied                               = 0x200F,
    PTP_RESPCODE_NoThumbnailPresent                         = 0x2010,
    PTP_RESPCODE_SelfTestFailed                             = 0x2011,
    PTP_RESPCODE_PartialDeletion                            = 0x2012,
    PTP_RESPCODE_StoreNotAvailable                          = 0x2013,
    PTP_RESPCODE_SpecificationByFormatUnsupported           = 0x2014,
    PTP_RESPCODE_NoValidObjectInfo                          = 0x2015,
    PTP_RESPCODE_InvalidCodeFormat                          = 0x2016,
    PTP_RESPCODE_UnknownVendorCode                          = 0x2017,
    PTP_RESPCODE_CaptureAlreadyTerminated                   = 0x2018,
    PTP_RESPCODE_DeviceBusy                                 = 0x2019,
    PTP_RESPCODE_InvalidParentObject                        = 0x201A,
    PTP_RESPCODE_InvalidDevicePropFormat                    = 0x201B,
    PTP_RESPCODE_InvalidDevicePropValue                     = 0x201C,
    PTP_RESPCODE_InvalidParameter                           = 0x201D,
    PTP_RESPCODE_SessionAlreadyOpen                         = 0x201E,
    PTP_RESPCODE_TransactionCancelled                       = 0x201F,
    PTP_RESPCODE_SpecificationofDestinationUnsupported      = 0x2020,
};

#define PTP_RESPCODE_IS_OK_ISH(x) (((x) == PTP_RESPCODE_OK) || ((x) == PTP_RESPCODE_OperationNotSupported) || ((x) == PTP_RESPCODE_InvalidStorageID) || ((x) == PTP_RESPCODE_StoreNotAvailable) || ((x) == PTP_RESPCODE_CaptureAlreadyTerminated))

enum
{
    PTP_EVENTCODE_CancelTransaction          = 0x4001,
    PTP_EVENTCODE_ObjectAdded                = 0x4002,
    PTP_EVENTCODE_ObjectRemoved              = 0x4003,
    PTP_EVENTCODE_StoreAdded                 = 0x4004,
    PTP_EVENTCODE_StoreRemoved               = 0x4005,
    PTP_EVENTCODE_DevicePropChanged          = 0x4006,
    PTP_EVENTCODE_ObjectInfoChanged          = 0x4007,
    PTP_EVENTCODE_DeviceInfoChanged          = 0x4008,
    PTP_EVENTCODE_RequestObjectTransfer      = 0x4009,
    PTP_EVENTCODE_StoreFull                  = 0x400A,
    PTP_EVENTCODE_StorageInfoChanged         = 0x400C,
    PTP_EVENTCODE_CaptureComplete            = 0x400D,
    PTP_EVENTCODE_ObjectAddedInSdram         = 0xC101,
    PTP_EVENTCODE_CaptureCompleteRecInSdram  = 0xC102,
    PTP_EVENTCODE_RecordingInterrupted       = 0xC105,
};

enum
{
    PTP_EXPOPROGMODE_UNDEFINED       = 0x0000,
    PTP_EXPOPROGMODE_MANUAL          = 0x0001,
    PTP_EXPOPROGMODE_AUTO            = 0x0002,
    PTP_EXPOPROGMODE_A               = 0x0003,
    PTP_EXPOPROGMODE_S               = 0x0004,
    PTP_EXPOPROGMODE_PROGRAMCREATIVE = 0x0005,
    PTP_EXPOPROGMODE_PROGRAMACTION   = 0x0006,
    PTP_EXPOPROGMODE_PORTRAIT        = 0x0007,
};

#endif
