#ifndef _PTPIP_SONYALPHACAM_H_
#define _PTPIP_SONYALPHACAM_H_

#include "PtpIpCamera.h"
#include "ptpsonycodes.h"

#define SONYCAM_DEFAULT_SHUTTER_TIME 250

enum
{
    SONYALPHAMODEL_NONE    = 0,
    SONYALPHAMODEL_A1      = 1,
    SONYALPHAMODEL_A7SM3,
    SONYALPHAMODEL_A9M2,
    SONYALPHAMODEL_A7M4A,
    SONYALPHAMODEL_A7RM4,
    SONYALPHAMODEL_A7C,
    SONYALPHAMODEL_A7M4,
    SONYALPHAMODEL_ZVE10,
    SONYALPHAMODEL_ZV1,
    SONYALPHAMODEL_A6600,
    SONYALPHAMODEL_A6500,
    SONYALPHAMODEL_A6400,
    SONYALPHAMODEL_A6300,
    SONYALPHAMODEL_A6200,
    SONYALPHAMODEL_A6100,
    SONYALPHAMODEL_A6000,
};

class PtpIpSonyAlphaCamera : public PtpIpCamera
{
    public:
        PtpIpSonyAlphaCamera(char* host_name, uint16_t* interested_properties);
        ptpipcam_prop_t* properties;
        int properties_cnt;
        volatile bool is_focused;

        uint8_t cam_model = 0;

        bool update_property(uint16_t prop_code, uint16_t data_type, uint8_t* data_chunk, uint8_t data_size);
        int32_t get_property(uint16_t prop_code);
        uint32_t get_property_enum(uint16_t prop_code, uint32_t cur_val, int32_t step = 0);
        bool has_property(uint16_t prop_code);
        bool check_dev_props(void);
        bool check_get_object(void);
        void decode_properties(void);
        void decode_objinfo(void);
        void test_prop_decode(uint8_t* data, uint32_t len);

        bool decode_pkt(uint8_t buff[], uint32_t buff_len);
        bool check_name(void);
        void task(void);

        bool is_manuallyfocused(void);
        bool is_movierecording(void);
        bool is_spotfocus(void);
        //bool is_focused(void);
        inline bool need_wait_af(void) { return is_manuallyfocused() == false; }
        bool is_continuousshooting(void);
        bool cmd_AutoFocus(bool onoff);
        bool cmd_Shutter(bool openclose);
        bool cmd_ManualFocusStep(int16_t step);
        bool cmd_ZoomStep(int16_t step);
        bool cmd_FocusPointSet(int16_t x, int16_t y);
        bool cmd_Shoot(int t);
        bool cmd_MovieRecord(bool onoff);
        bool cmd_MovieRecordToggle();
        bool cmd_ManualFocusMode(bool onoff, bool precheck = false);
        bool cmd_ManualFocusToggle(bool onoff);
        bool cmd_ShutterSpeedStep(int8_t step);
        //bool cmd_ShutterSpeedSet(int16_t numerator, int16_t denominator); // disabled, unfriendly API
        bool cmd_ShutterSpeedSet32(uint32_t x);  // input parameter expects a 16 bit numerator and 16 bit denominator, and must be an exact match for an item from the corresponding enum list
        bool cmd_IsoSet(uint32_t x);             // input parameter expects value to be a positive integer, and must be an exact match for an item from the corresponding enum list, meaning the "double bar" flag must exist for some items
        bool cmd_ApertureSet(uint16_t x);        // input parameter expects value to be x * 1000 where x is the f/num, must be an exact match for an item from the corresponding enum list
        bool cmd_ExpoCompSet(int32_t x);         // input parameter expects value to be x * 1000 where x is stops of light, but with 2nd decimal place 0, example: +1.7 ev must be specified as 1700, 1666 should be rounded to 1700
        bool cmd_arb(uint32_t opcode, uint32_t propcode, uint8_t* payload, uint32_t payload_len);

        #if 0
        bool get_jpg(void (*cb_s)(uint8_t*, uint32_t), void (*cb_d)(void));
        #endif

        bool need_check_properties;
        bool need_check_object;
        uint32_t need_check_object_id;
        bool properties_pending;
        bool objinfo_pending;
        uint32_t check_props_time;
        uint16_t* p_interested_properties;

        bool propdecode_weird_string;
        bool propdecode_weird_form;

        // these tables, if they are not NULL, will contain valid entries that the camera accepts
        // index 0 is always the length of the table
        // the rest of the table might not be 32 bit per entry, for example, aperture is actually 16 bits per entry, so do "uint16* ptr = (uint16_t*)(&table_aperture[1]);"
        uint32_t* table_shutter_speed;
        uint32_t* table_iso;
        uint32_t* table_aperture;
};

#endif
