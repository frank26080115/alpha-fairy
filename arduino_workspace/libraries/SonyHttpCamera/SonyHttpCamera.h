#ifndef _SONYHTTPCAMERA_H_
#define _SONYHTTPCAMERA_H_

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <string.h>

#include <DebuggingSerial.h>

#include <WiFiUdp.h>
#include <HTTPClient.h>

#define SHCAM_RXBUFF_UNIT     64

#ifndef DEFAULT_BUSY_TIMEOUT
#define DEFAULT_BUSY_TIMEOUT  5000
#endif

#define SHCAM_NEED_ENTER_MOVIE_MODE

enum
{
    SHCAMSTATE_NONE                = 0,
    SHCAMSTATE_WAIT                = 1,
    SHCAMSTATE_CONNECTING          = 2,
    SHCAMSTATE_INIT_SSDP           = 4,
    SHCAMSTATE_INIT_GETDD          = 5,
    SHCAMSTATE_INIT_GOTDD          = 6,
    SHCAMSTATE_INIT_ACCESSCONTROL  = 8,
    SHCAMSTATE_INIT_GETVERSION     = 10,
    SHCAMSTATE_INIT_GETAPILIST     = 12,
    SHCAMSTATE_INIT_STARTRECMODE   = 14,
    SHCAMSTATE_INIT_SETCAMFUNC     = 16,
    SHCAMSTATE_INIT_DONE1,
    SHCAMSTATE_INIT_DONE2          = 20,
    SHCAMSTATE_READY               = 22,
    SHCAMSTATE_POLLING             = 24,
    SHCAMSTATE_COMMANDING          = 26,
    SHCAMSTATE_FAILED              = 0x80,
    SHCAMSTATE_DISCONNECTED        = 0x81,
    SHCAMSTATE_FORBIDDEN           = 0x82,
};

enum
{
    DEBUGFLAG_NONE   = 0x00,
    DEBUGFLAG_STATES = 0x01,
    DEBUGFLAG_EVENTS = 0x02,
    DEBUGFLAG_RX     = 0x04,
    DEBUGFLAG_TX     = 0x08,
    DEBUGFLAG_DEVPROP_DUMP   = 0x10,
    DEBUGFLAG_DEVPROP_CHANGE = 0x20,
};

enum
{
    SHOOTMODE_STILLS,
    SHOOTMODE_MOVIE,
};

enum
{
    SHCAM_FOCUSMODE_NONE = 0,
    SHCAM_FOCUSMODE_MF,
    SHCAM_FOCUSMODE_AF,
};

bool scan_json_for_key(char* data, int32_t datalen, const char* keystr, signed int* start_idx, signed int* end_idx, char* tgt, int tgtlen);
int count_commas(char* data);
void strcpy_no_slash(char* dst, char* src);
bool get_txt_within_strtbl(char* tbl, int idx, char* tgt);
int get_idx_within_strtbl(char* tbl, char* needle);
uint32_t parse_shutter_speed_str(char* s);
bool parse_json_err_num(const char* data, int* outnum);

class SonyHttpCamera
{
    public:
        SonyHttpCamera();

        void begin(uint32_t ip, WiFiUDP* udpsock = NULL);
        void poll(void);
        void task(void);

        inline uint32_t  getIp           (void) { return ip_addr; };
        inline char*     getCameraName   (void) { return friendly_name; };
        inline uint8_t   getState        (void) { return state; };
        inline bool      canSend         (void) { return state >= SHCAMSTATE_READY && (state & 1) == 0; };
        inline bool      isOperating     (void) { return state >= SHCAMSTATE_READY && state < SHCAMSTATE_FAILED; };
        inline bool      canNewConnect   (void) { return state == SHCAMSTATE_NONE || state == SHCAMSTATE_FAILED || state == SHCAMSTATE_DISCONNECTED; };
        inline void      setForbidden    (void) { state = SHCAMSTATE_FORBIDDEN; }
               void      force_disconnect(void);

        inline int       getPollDelay    (void)       { return poll_delay; };
        inline void      setPollDelay    (uint32_t x) { poll_delay = x; };
        inline void      setPollDelaySlow(void)       { poll_delay = 500; };

        inline bool      is_movierecording (void) { return is_movierecording_v; };
        inline uint8_t   is_manuallyfocused(void) { return is_manuallyfocused_v; };
        bool             is_focused;
        inline bool      is_moviemode      (void) { return shoot_mode == SHOOTMODE_MOVIE; };
        inline bool      need_wait_af      (void) { return has_focus_status && is_manuallyfocused_v == SHCAM_FOCUSMODE_AF; };

        inline char*     getLiveviewUrl(void) { return liveview_url; };

        void disconnect(void) { state = SHCAMSTATE_DISCONNECTED; };
        void wait_while_busy(uint32_t min_wait, uint32_t max_wait, volatile bool* exit_signal = NULL);

        void (*cb_onConnect)(void) = NULL;
        void (*cb_onDisconnect)(void) = NULL;
        void (*cb_onCriticalError)(void) = NULL;
        void (*cb_onNoServiceUrl)(void) = NULL;

        uint32_t critical_error_cnt = 0;

        char* tbl_shutterspd = NULL;
        char* tbl_iso = NULL;
        char* tbl_aperture = NULL;

        void borrowBuffer(char*, uint32_t);
        inline void set_ssdpTimeout(uint32_t x) { ssdp_allowed_time = x; };

        virtual void set_debugflags(uint32_t x);
        uint32_t debug_flags;
        void test_debug_msg(const char*);

    protected:
        uint32_t ip_addr;
        uint8_t  state, state_after_wait;
        uint32_t req_id;
        uint32_t wait_until;
        uint32_t start_time;
        uint32_t ssdp_allowed_time = 5;
        bool need_disconnect = false;

        char friendly_name[256];
        char service_url[256];
        char access_url[256];
        char url_buffer[256];
        char cmd_buffer[256];
        char* liveview_url;

        char*    rx_buff = NULL;
        uint32_t rx_buff_idx;
        static uint32_t rx_buff_size;

        HTTPClient httpclient;
        int32_t    http_content_len;
        static int read_in_chunk(WiFiClient* stream, int32_t chunk, char* buff, uint32_t* buff_idx);
        WiFiUDP* ssdp_udp;

        uint32_t init_retries;
        uint32_t error_cnt;
        uint8_t  event_api_version;
        int last_http_resp_code;

        uint32_t last_poll_time;
        uint32_t poll_delay;

        char str_shutterspd[32];
        char str_shutterspd_clean[32];
        char str_iso[32];
        char str_focusstatus[32];
        char str_afmode[32];
        char str_aperture[32];
        char str_aperture_prev[32];
        char str_expocomp[32];
        char str_expomode[32];
        int  expocomp;

        int8_t   zoom_state;
        uint32_t zoom_time;
        bool     is_movierecording_v;
        uint8_t  is_manuallyfocused_v;
        uint8_t  shoot_mode;
        bool     has_focus_status;

        bool parse_event(char* data, int32_t maxlen = 0);
        void parse_dd_xml(char* data, int32_t maxlen = 0);
        void get_event(void);
        void get_dd_xml(void);
        uint32_t event_found_flag;

        void ssdp_start(WiFiUDP* sock);
        bool ssdp_checkurl(WiFiUDP* sock);
        bool ssdp_poll(WiFiUDP* sock);
        void cmd_prep(void);
        bool cmd_send(char* data, char* alt_url = NULL, bool callend = true);

        static const char cmd_generic_fmt[];
        static const char cmd_generic_strparam_fmt[];
        static const char cmd_generic_strintparam_fmt[];
        static const char cmd_generic_strfloatparam_fmt[];
        static const char cmd_generic_intparam_fmt[];
        static const char cmd_generic_floatparam_fmt[];
        static const char cmd_zoom_fmt[];

        static DebuggingSerial* dbgser_important;
        static DebuggingSerial* dbgser_states;
        static DebuggingSerial* dbgser_events;
        static DebuggingSerial* dbgser_rx;
        static DebuggingSerial* dbgser_tx;
        static DebuggingSerial* dbgser_devprop_dump;
        static DebuggingSerial* dbgser_devprop_change;

    public:
        inline char*    get_shutterspd_str  (void) { return str_shutterspd_clean; };
        inline uint32_t get_shutterspd_32   (void) { return parse_shutter_speed_str(str_shutterspd_clean); };
        inline int      get_shutterspd_idx  (void) { return (tbl_shutterspd != NULL) ? get_idx_within_strtbl(tbl_shutterspd, str_shutterspd) : -1; };
        inline char*    get_iso_str         (void) { return str_iso; };
        inline int      get_iso_idx         (void) { return (tbl_iso != NULL) ? get_idx_within_strtbl(tbl_iso, str_iso) : -1; };
        inline char*    get_aperture_str    (void) { return str_aperture; };
        inline int      get_aperture_idx    (void) { return (tbl_aperture != NULL) ? get_idx_within_strtbl(tbl_aperture, str_aperture) : -1; };

        uint32_t get_another_shutterspd(int idx, char* tgt);

        inline char* get_str_afmode  (void) { return str_afmode  ; };
        inline char* get_str_aperture(void) { return str_aperture; };
        inline char* get_str_expocomp(void) { return str_expocomp; };
        inline char* get_str_expomode(void) { return str_expomode; };
        inline int   get_expocomp    (void) { return expocomp;     };

        void cmd_Shoot(void);
        void cmd_MovieRecord(bool is_start);
        void cmd_MovieRecordToggle(void);
        void cmd_ZoomStart(int dir);
        void cmd_ZoomStop(void);
        void cmd_FocusPointSet16(int16_t x, int16_t y);
        void cmd_FocusPointSetF(float x, float y);

        void cmd_ShutterSpeedSetStr(char*); // input parameter must match string from "shutterSpeedCandidates"
        void cmd_IsoSet(uint32_t x);        // input parameter must match format from PTP mode
        void cmd_IsoSetStr(char*);          // input parameter must match string from "isoSpeedRateCandidates"
        void cmd_ApertureSet(float x);      // input parameter must match string from "fNumberCandidates" after string formatting with "%0.1f"
        void cmd_ApertureSet32(uint32_t x); // input parameter must match format from PTP mode
        void cmd_ApertureSetStr(char*);     // input parameter must match string from "fNumberCandidates"
        void cmd_ExpoCompSet32(int32_t x);  // input parameter must match format from PTP mode
        void cmd_ExpoCompSetIdx(int32_t x); // input parameter is a signed integer number, each step represents 1/3 of a stop

        void cmd_ManualFocusMode(bool onoff, bool precheck = false);
        void cmd_ManualFocusToggle(bool onoff);
        void cmd_AutoFocus(bool onoff);
        #ifdef SHCAM_NEED_ENTER_MOVIE_MODE
        void cmd_MovieMode(bool onoff);
        #endif
};

#endif
