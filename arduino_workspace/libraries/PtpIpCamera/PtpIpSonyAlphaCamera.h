#ifndef _PTPIP_SONYALPHACAM_H_
#define _PTPIP_SONYALPHACAM_H_

#include "PtpIpCamera.h"
#include "ptpsonycodes.h"

#define SONYCAM_DEFAULT_SHUTTER_TIME 250

enum
{
    SONYALPHAMODEL_NONE = 0,
    SONYALPHAMODEL_A1   = 1,
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
        bool has_property(uint16_t prop_code);
        bool check_dev_props(void);
        void decode_properties(void);
        void test_prop_decode(uint8_t* data, uint32_t len);

        bool decode_pkt(uint8_t buff[], uint32_t buff_len);
        void task(void);

        bool is_manuallyfocused(void);
        bool is_movierecording(void);
        bool is_spotfocus(void);
        //bool is_focused(void);
        bool is_continuousshooting(void);
        bool cmd_AutoFocus(bool onoff);
        bool cmd_Shutter(bool openclose);
        bool cmd_ManualFocusStep(int16_t step);
        bool cmd_FocusPointSet(int16_t x, int16_t y);
        bool cmd_Shoot(int t);
        bool cmd_MovieRecord(bool onoff);
        bool cmd_MovieRecordToggle();
        bool cmd_ManualFocusMode(bool onoff);
        bool cmd_ManualFocusToggle(bool onoff);
        bool cmd_ShutterSpeedStep(int8_t step);
        bool cmd_ShutterSpeedSet(int16_t numerator, int16_t denominator);
        bool cmd_ShutterSpeedSet32(uint32_t x);

        bool need_check_properties;
        bool properties_pending;
        uint32_t check_props_time;
        uint16_t* p_interested_properties;

        bool propdecode_weird_string;
        bool propdecode_weird_form;

        uint32_t* table_shutter_speed;
};

#endif
