PKTTYPE_INITCMDREQ                  = 0x0001
PKTTYPE_INITCMDACK                  = 0x0002
PKTTYPE_INITEVENTREQ                = 0x0003
PKTTYPE_INITEVENTACK                = 0x0004
PKTTYPE_INITFAILED                  = 0x0005
PKTTYPE_OPERREQ                     = 0x0006
PKTTYPE_OPERRESP                    = 0x0007
PKTTYPE_EVENT                       = 0x0008
PKTTYPE_STARTDATA                   = 0x0009
PKTTYPE_DATA                        = 0x000A
PKTTYPE_CANCELDATA                  = 0x000B
PKTTYPE_ENDDATA                     = 0x000C
PKTTYPE_PROBEREQ                    = 0x000D
PKTTYPE_PROBERESP                   = 0x000E

OPCODE_Undefined                    = 0x1000
OPCODE_GetDeviceInfo                = 0x1001
OPCODE_OpenSession                  = 0x1002
OPCODE_CloseSession                 = 0x1003
OPCODE_GetStorageIDs                = 0x1004
OPCODE_GetStorageInfo               = 0x1005
OPCODE_GetNumObjects                = 0x1006
OPCODE_GetObjectHandles             = 0x1007
OPCODE_GetObjectInfo                = 0x1008
OPCODE_GetObject                    = 0x1009
OPCODE_GetThumb                     = 0x100A
OPCODE_DeleteObject                 = 0x100B
OPCODE_SendObjectInfo               = 0x100C
OPCODE_SendObject                   = 0x100D
OPCODE_InitiateCapture              = 0x100E
OPCODE_FormatStore                  = 0x100F
OPCODE_ResetDevice                  = 0x1010
OPCODE_SelfTest                     = 0x1011
OPCODE_SetObjectProtection          = 0x1012
OPCODE_PowerDown                    = 0x1013
OPCODE_GetDevicePropDesc            = 0x1014
OPCODE_GetDevicePropValue           = 0x1015
OPCODE_SetDevicePropValue           = 0x1016
OPCODE_ResetDevicePropValue         = 0x1017
OPCODE_TerminateOpenCapture         = 0x1018
OPCODE_MoveObject                   = 0x1019
OPCODE_CopyObject                   = 0x101A
OPCODE_GetPartialObject             = 0x101B
OPCODE_InitiateOpenCapture          = 0x101C
OPCODE_GetObjectPropValue           = 0x9803

CONTAINER_UNDEFINED                 = 0x0000
CONTAINER_CMD                       = 0x0001
CONTAINER_DATA                      = 0x0002
CONTAINER_RESPONSE                  = 0x0003
CONTAINER_EVENT                     = 0x0004

PROPCODE_BatteryLevel               = 0x5001
PROPCODE_FunctionalMode             = 0x5002
PROPCODE_ImageSize                  = 0x5003
PROPCODE_CompressionSetting         = 0x5004
PROPCODE_WhiteBalance               = 0x5005
PROPCODE_RGBGain                    = 0x5006
PROPCODE_Aperture                   = 0x5007
PROPCODE_FocalLength                = 0x5008
PROPCODE_FocusDistance              = 0x5009
PROPCODE_FocusMode                  = 0x500A
PROPCODE_ExposureMeteringMode       = 0x500B
PROPCODE_FlashMode                  = 0x500C
PROPCODE_ExposureTime               = 0x500D
PROPCODE_ExposureProgramMode        = 0x500E
PROPCODE_ExposureIndex              = 0x500F
PROPCODE_ExposureBiasCompensation   = 0x5010
PROPCODE_DateTime                   = 0x5011
PROPCODE_CaptureDelay               = 0x5012
PROPCODE_StillCaptureMode           = 0x5013
PROPCODE_Contrast                   = 0x5014
PROPCODE_Sharpness                  = 0x5015
PROPCODE_DigitalZoom                = 0x5016
PROPCODE_EffectMode                 = 0x5017
PROPCODE_BurstNumber                = 0x5018
PROPCODE_BurstInterval              = 0x5019
PROPCODE_TimelapseNumber            = 0x501A
PROPCODE_TimelapseInterval          = 0x501B
PROPCODE_FocusMeteringMode          = 0x501C
PROPCODE_UploadURL                  = 0x501D
PROPCODE_Artist                     = 0x501E
PROPCODE_CopyrightInfo              = 0x501F

RESPCODE_Undefined                                  = 0x2000
RESPCODE_OK                                         = 0x2001
RESPCODE_GeneralError                               = 0x2002
RESPCODE_SessionNotOpen                             = 0x2003
RESPCODE_InvalidTransactionID                       = 0x2004
RESPCODE_OperationNotSupported                      = 0x2005
RESPCODE_ParameterNotSupported                      = 0x2006
RESPCODE_IncompleteTransfer                         = 0x2007
RESPCODE_InvalidStorageID                           = 0x2008
RESPCODE_InvalidObjectHandle                        = 0x2009
RESPCODE_DevicePropNotSupported                     = 0x200A
RESPCODE_InvalidObjectFormatCode                    = 0x200B
RESPCODE_StoreFull                                  = 0x200C
RESPCODE_ObjectWriteProtected                       = 0x200D
RESPCODE_StoreReadOnly                              = 0x200E
RESPCODE_AccessDenied                               = 0x200F
RESPCODE_NoThumbnailPresent                         = 0x2010
RESPCODE_SelfTestFailed                             = 0x2011
RESPCODE_PartialDeletion                            = 0x2012
RESPCODE_StoreNotAvailable                          = 0x2013
RESPCODE_SpecificationByFormatUnsupported           = 0x2014
RESPCODE_NoValidObjectInfo                          = 0x2015
RESPCODE_InvalidCodeFormat                          = 0x2016
RESPCODE_UnknownVendorCode                          = 0x2017
RESPCODE_CaptureAlreadyTerminated                   = 0x2018
RESPCODE_DeviceBusy                                 = 0x2019
RESPCODE_InvalidParentObject                        = 0x201A
RESPCODE_InvalidDevicePropFormat                    = 0x201B
RESPCODE_InvalidDevicePropValue                     = 0x201C
RESPCODE_InvalidParameter                           = 0x201D
RESPCODE_SessionAlreadyOpen                         = 0x201E
RESPCODE_TransactionCancelled                       = 0x201F
RESPCODE_SpecificationofDestinationUnsupported      = 0x2020

RESPCODE_OKish = [RESPCODE_OK, RESPCODE_OperationNotSupported, RESPCODE_InvalidStorageID, RESPCODE_StoreNotAvailable, RESPCODE_CaptureAlreadyTerminated]

EVENTCODE_CancelTransaction          = 0x4001
EVENTCODE_ObjectAdded                = 0x4002
EVENTCODE_ObjectRemoved              = 0x4003
EVENTCODE_StoreAdded                 = 0x4004
EVENTCODE_StoreRemoved               = 0x4005
EVENTCODE_DevicePropChanged          = 0x4006
EVENTCODE_ObjectInfoChanged          = 0x4007
EVENTCODE_DeviceInfoChanged          = 0x4008
EVENTCODE_RequestObjectTransfer      = 0x4009
EVENTCODE_StoreFull                  = 0x400A
EVENTCODE_StorageInfoChanged         = 0x400C
EVENTCODE_CaptureComplete            = 0x400D
EVENTCODE_ObjectAddedInSdram         = 0xC101
EVENTCODE_CaptureCompleteRecInSdram  = 0xC102
EVENTCODE_RecordingInterrupted       = 0xC105

def get_name_(val, mod):
    lst = dir(mod)
    for i in lst:
        try:
            x = getattr(mod, i)
            if int(val) == int(x):
                if "_" in i:
                    i = i[i.index('_') + 1:]
                return i
        except Exception as ex:
            pass
    return None

def get_name(val):
    try:
        import sys
        x = get_name_(val, sys.modules[__name__])
        if x is not None:
            return x
        import ptpsonycodes
        x = get_name_(val, ptpsonycodes)
        if x is not None:
            return x
        return "0x%04X" % val
    except:
        pass
    return "0x%04X" % val