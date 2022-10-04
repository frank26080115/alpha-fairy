#include "PtpIpSonyAlphaCamera.h"
#include <Arduino.h>
#include "ptpip_utils.h"

#include <stdlib.h>

#define SONYCAM_CHECK_PROPS_INTERVAL 10000

static const ptpip_init_substep_t init_table[] = {
    { PTP_OPCODE_GetDeviceInfo             , {     0,  0, 0 } , 0 },
    { PTP_OPCODE_GetStorageIDs             , {     0,  0, 0 } , 0 },
    { SONYALPHA_OPCODE_SDIOConnect         , {     1,  0, 0 } , 3 },
    { SONYALPHA_OPCODE_SDIOConnect         , {     2,  0, 0 } , 3 },
    { SONYALPHA_OPCODE_SDIOGetExtDeviceInfo, { 0x12C,  0, 0 } , 3 },
    { SONYALPHA_OPCODE_SDIOConnect         , {     3,  0, 0 } , 3 },
    { SONYALPHA_OPCODE_SDIOGetExtDeviceInfo, { 0x12C,  0, 0 } , 3 },
    {                                     0, {     0,  0, 0 } , 0 }, // end of table
};

static const uint16_t interested_properties_default[] = {
    SONYALPHA_PROPCODE_FocusFound,
    SONYALPHA_PROPCODE_FocusMode,
    SONYALPHA_PROPCODE_Recording,
    SONYALPHA_PROPCODE_FocusArea,
    SONYALPHA_PROPCODE_ManualFocusMode,
    SONYALPHA_PROPCODE_ManualFocusDist,
    SONYALPHA_PROPCODE_ZoomStep,
    SONYALPHA_PROPCODE_ShutterSpeed,
    SONYALPHA_PROPCODE_DriveMode,
    SONYALPHA_PROPCODE_ISO,
    #ifdef SONYCAM_PROPCODE_NEED_MORE
    SONYALPHA_PROPCODE_FocusPointGet,
    SONYALPHA_PROPCODE_FocusPointSet,
    SONYALPHA_PROPCODE_ExposeIndex,
    SONYALPHA_PROPCODE_ExpoComp,
    SONYALPHA_PROPCODE_Aperture,
    SONYALPHA_PROPCODE_FileFormat,
    SONYALPHA_PROPCODE_ObjectInMemory,
    SONYALPHA_PROPCODE_SonyBatteryLevel,
    SONYALPHA_PROPCODE_MemoryRemaining_Card1,
    SONYALPHA_PROPCODE_MemoryRemaining_Card2,
    PTP_PROPCODE_ExposureProgramMode,
    #endif
    0x0000, // end of table
};

PtpIpSonyAlphaCamera::PtpIpSonyAlphaCamera(char* host_name, uint16_t* interested_properties) : PtpIpCamera(host_name) {
    // make the properties table based on how many properties we are interested in
    uint8_t i, j;
    // so count first
    for (i = 0; i < 255; i++) {
        if (interested_properties_default[i] == 0) { // end of table
            break;
        }
    }
    if (interested_properties != NULL) {
        for (j = 0; j < 255 ; j++) {
            if (interested_properties[j] == 0) { // end of table
                break;
            }
        }
    }
    properties_cnt = i + j;
    properties = (ptpipcam_prop_t*)malloc(properties_cnt * sizeof(ptpipcam_prop_t));
    memset(properties, 0, sizeof(ptpipcam_prop_t) * properties_cnt);
    p_interested_properties = (uint16_t*)malloc(properties_cnt * sizeof(uint16_t));
    memcpy(p_interested_properties, interested_properties_default, sizeof(uint16_t) * i);
    if (interested_properties != NULL && j > 0) {
        memcpy(&(p_interested_properties[i]), interested_properties, sizeof(uint16_t) * j);
    }

    this->init_substeps = (ptpip_init_substep_t*)init_table;

    propdecode_weird_string = true;
    propdecode_weird_form   = true;

    table_shutter_speed = NULL;

    check_props_time = 0;
    need_check_properties = false;
    properties_pending = false;
}

bool PtpIpSonyAlphaCamera::check_name()
{
    this->cam_model = SONYALPHAMODEL_A1;
    // TODO: assign model number according to name string
    if (camera_name_check(this->cam_name, "ILCE-1")) {
        this->cam_model = SONYALPHAMODEL_A1;
    }
    else if (camera_name_check(this->cam_name, "A7M4A") || camera_name_check(this->cam_name, "ILCE-7M4A")) {
        this->cam_model = SONYALPHAMODEL_A7M4A;
    }
    else if (camera_name_check(this->cam_name, "A7M4") || camera_name_check(this->cam_name, "ILCE-7M4")) {
        this->cam_model = SONYALPHAMODEL_A7M4;
    }
    else if (camera_name_check(this->cam_name, "A7SM3") || camera_name_check(this->cam_name, "ILCE-7SM3")) {
        this->cam_model = SONYALPHAMODEL_A7SM3;
    }
    else if (camera_name_check(this->cam_name, "A9M2") || camera_name_check(this->cam_name, "ILCE-9M2")) {
        this->cam_model = SONYALPHAMODEL_A9M2;
    }
    else if (camera_name_check(this->cam_name, "A7RM4") || camera_name_check(this->cam_name, "ILCE-7M4")) {
        this->cam_model = SONYALPHAMODEL_A7RM4;
    }
    else if (camera_name_check(this->cam_name, "A7C") || camera_name_check(this->cam_name, "ILCE-7C")) {
        this->cam_model = SONYALPHAMODEL_A7C;
    }
    else if (camera_name_check(this->cam_name, "ZV-E10") || camera_name_check(this->cam_name, "ZVE10")) { // confirmed "ZV-E10"
        this->cam_model = SONYALPHAMODEL_ZVE10;
    }
    else if (camera_name_check(this->cam_name, "ZV1") || camera_name_check(this->cam_name, "ZV-1")) {
        this->cam_model = SONYALPHAMODEL_ZV1;
    }
    else if (camera_name_check(this->cam_name, "A6600") || camera_name_check(this->cam_name, "ILCE-6600")) {
        this->cam_model = SONYALPHAMODEL_A6600;
    }
    else if (camera_name_check(this->cam_name, "A6500") || camera_name_check(this->cam_name, "ILCE-6500")) {
        this->cam_model = SONYALPHAMODEL_A6500;
    }
    else if (camera_name_check(this->cam_name, "A6400") || camera_name_check(this->cam_name, "ILCE-6400")) {
        this->cam_model = SONYALPHAMODEL_A6400;
    }
    else if (camera_name_check(this->cam_name, "A6300") || camera_name_check(this->cam_name, "ILCE-6300")) {
        this->cam_model = SONYALPHAMODEL_A6300;
    }
    else if (camera_name_check(this->cam_name, "A6200") || camera_name_check(this->cam_name, "ILCE-6200")) {
        this->cam_model = SONYALPHAMODEL_A6200;
    }
    else if (camera_name_check(this->cam_name, "A6100") || camera_name_check(this->cam_name, "ILCE-6100")) {
        this->cam_model = SONYALPHAMODEL_A6100;
    }
    else if (camera_name_check(this->cam_name, "A6000") || camera_name_check(this->cam_name, "ILCE-6000")) {
        this->cam_model = SONYALPHAMODEL_A6000;
    }
    return true;
}

bool PtpIpSonyAlphaCamera::decode_pkt(uint8_t buff[], uint32_t buff_len)
{
    ptpip_pkthdr_t* hdr = (ptpip_pkthdr_t*)buff;
    uint32_t pkt_len = hdr->length;
    uint32_t pkt_type = hdr->pkt_type;
    if (pkt_type == PTP_PKTTYPE_OPERRESP && state >= PTPSTATE_POLLING)
    {
        state &= 0xFFFFFFFE;
        if (properties_pending != false) {
            decode_properties();
        }
    }
    else if (pkt_type == PTP_PKTTYPE_ENDDATA)
    {
        if (properties_pending != false) {
            decode_properties();
        }
    }
    else if (pkt_type == PTP_PKTTYPE_EVENT) {
        ptpip_pkt_event_t* pktstruct = (ptpip_pkt_event_t*)buff;
        uint16_t event_code = pktstruct->event_code;
        if (event_code == SONYALPHA_EVENTCODE_PropertyChanged) {
            need_check_properties = true;
        }
    }
    return PtpIpCamera::decode_pkt(buff, buff_len);
}

bool PtpIpSonyAlphaCamera::check_dev_props()
{
    bool success;
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    need_check_properties = false;
    success = send_oper_req(SONYALPHA_OPCODE_GetAllDevicePropData, NULL, 0, NULL, -1);
    if (success != false) {
        check_props_time = millis();
        properties_pending = true;
    }
    return success;
}

#ifdef PTPIP_ENABLE_STREAMING
bool PtpIpSonyAlphaCamera::get_jpg(void (*cb_s)(uint8_t*, uint32_t), void (*cb_d)(void))
{
    bool success;
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    uint32_t objcode = 0xFFFFC002;
    success = send_oper_req(PTP_OPCODE_GetObject, &objcode, 1, NULL, 0);
    if (success != false) {
        reset_buffers();
        start_stream(cb_s, cb_d);
    }
    return success;
}
#endif

void PtpIpSonyAlphaCamera::task()
{
    yield();
    if (state >= PTPSTATE_POLLING && state < PTPSTATE_DISCONNECT)
    {
        uint32_t now = millis();
        #ifdef SONYCAM_CHECK_PROPS_INTERVAL
        #if SONYCAM_CHECK_PROPS_INTERVAL > 0
        if ((now - check_props_time) >= SONYCAM_CHECK_PROPS_INTERVAL) {
            need_check_properties |= true;
        }
        #endif
        #endif
        if (need_check_properties != false && canSend()) {
            check_dev_props();
        }
    }
    PtpIpCamera::task();
}

bool PtpIpSonyAlphaCamera::is_manuallyfocused()
{
    uint16_t prop_code = SONYALPHA_PROPCODE_FocusMode;
    if (has_property(prop_code) == false) {
        return false;
    }
    int32_t x = get_property(prop_code);
    return (x == 1);
}

bool PtpIpSonyAlphaCamera::is_movierecording()
{
    uint16_t prop_code = SONYALPHA_PROPCODE_Recording;
    if (has_property(prop_code) == false) {
        return false;
    }
    int32_t x = get_property(prop_code);
    return (x != 0);
}

bool PtpIpSonyAlphaCamera::is_spotfocus()
{
    uint16_t prop_code = SONYALPHA_PROPCODE_FocusArea;
    if (has_property(prop_code) == false) {
        return false;
    }
    int32_t x = get_property(prop_code);
    return (x == SONYALPHA_FOCUSAREA_ZONE || x == SONYALPHA_FOCUSAREA_TRACKING_ZONE || x == SONYALPHA_FOCUSAREA_MOVEABLE_SMALL  || x == SONYALPHA_FOCUSAREA_MOVEABLE_MEDIUM || x == SONYALPHA_FOCUSAREA_MOVEABLE_LARGE || x == SONYALPHA_FOCUSAREA_MOVEABLE_EXPAND || x == SONYALPHA_FOCUSAREA_TRACKING_MOVEABLE_SMALL || x == SONYALPHA_FOCUSAREA_TRACKING_MOVEABLE_MEDIUM || x == SONYALPHA_FOCUSAREA_TRACKING_MOVEABLE_LARGE || x == SONYALPHA_FOCUSAREA_TRACKING_MOVEABLE_EXPAND);
}

bool PtpIpSonyAlphaCamera::is_continuousshooting()
{
    uint16_t prop_code = SONYALPHA_PROPCODE_DriveMode;
    if (has_property(prop_code) == false) {
        return false;
    }
    int32_t x = get_property(prop_code);
    return (x == SONYALPHA_DRIVEMODE_ContHi || x == SONYALPHA_DRIVEMODE_ContHiPlus || x == SONYALPHA_DRIVEMODE_ContLo || x == SONYALPHA_DRIVEMODE_ContMid || x == SONYALPHA_DRIVEMODE_Continuous || x == SONYALPHA_DRIVEMODE_ContinuousSpeedPriority);
}

/*
//this function is available as a live-updated variable instead

bool PtpIpSonyAlphaCamera::is_focused()
{
    uint16_t prop_code = SONYALPHA_PROPCODE_FocusFound;
    if (has_property(prop_code) == false) {
        return false;
    }
    int32_t x = get_property(prop_code);
    return (x != SONYALPHA_FOCUSSTATUS_FOCUSED);
}
*/
