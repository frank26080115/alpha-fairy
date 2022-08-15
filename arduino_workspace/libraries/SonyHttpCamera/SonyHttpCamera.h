#ifndef _SONYHTTPCAMERA_H_
#define _SONYHTTPCAMERA_H_

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <string.h>

#include <DebuggingSerial.h>

#include "AsyncHTTPRequest_Generic.hpp"

#define SHCAM_RXBUFF_UNIT     64
#define SHCAM_RXBUFF_UNIT_CNT (4 * 4)
#define SHCAM_RXBUFF_SIZE     (SHCAM_RXBUFF_UNIT * SHCAM_RXBUFF_UNIT_CNT)

#ifndef DEFAULT_BUSY_TIMEOUT
#define DEFAULT_BUSY_TIMEOUT  5000
#endif

enum
{
    SHCAMSTATE_NONE                = 0,
    SHCAMSTATE_CONNECTING          = 1,
    SHCAMSTATE_INIT_GOTDD          = 2,
    SHCAMSTATE_INIT_STARTRECMODE   = 4,
    SHCAMSTATE_INIT_SETCAMFUNC     = 6,
    SHCAMSTATE_INIT_DONE           = 8,
    SHCAMSTATE_READY               = 12,
    SHCAMSTATE_POLLING             = 13,
    SHCAMSTATE_COMMANDING          = 15,
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

bool scan_json_for_key(char* data, int32_t datalen, const char* keystr, signed int* start_idx, signed int* end_idx, char* tgt, int tgtlen);
int count_commas(char* data);
void strcpy_no_slash(char* dst, char* src);
bool get_txt_within_strtbl(char* tbl, int idx, char* tgt);
int get_idx_within_strtbl(char* tbl, char* needle);
uint32_t parse_shutter_speed_str(char* s);

class SonyHttpCamera
{
    public:
        SonyHttpCamera();
        void begin(uint32_t ip);
        void poll(void);
        void task(void);

        inline uint32_t  getIp           (void) { return ip_addr; };
        inline char*     getCameraName   (void) { return friendly_name; };
        inline uint8_t   getState        (void) { return state; };
        inline bool      canSend         (void) { return state >= SHCAMSTATE_READY && (state & 1) == 0; };
        inline bool      isOperating     (void) { return state >= SHCAMSTATE_READY && state < SHCAMSTATE_FAILED; };
        inline bool      canNewConnect   (void) { return state == SHCAMSTATE_NONE || state == SHCAMSTATE_FAILED || state == SHCAMSTATE_DISCONNECTED; };
        inline void      setForbidden    (void) { state = SHCAMSTATE_FORBIDDEN; }

        inline int       getPollDelay    (void)       { return poll_delay; };
        inline void      setPollDelay    (uint32_t x) { poll_delay = x; };
        inline void      setPollDelaySlow(void)       { poll_delay = 500; };

        inline bool      is_movierecording (void) { return is_movierecording_v; };
        inline bool      is_manuallyfocused(void) { return is_manuallyfocused_v; };
        bool             is_focused;

        inline char*     getLiveviewUrl(void) { return liveview_url; };

        void disconnect(void) { state = SHCAMSTATE_DISCONNECTED; };
        void wait_while_busy(uint32_t min_wait, uint32_t max_wait, volatile bool* exit_signal = NULL);

        void (*cb_onConnect)(void) = NULL;
        void (*cb_onDisconnect)(void) = NULL;

        virtual void set_debugflags(uint32_t x);
        uint32_t debug_flags;
        void test_debug_msg(const char*);

    protected:
        uint32_t ip_addr;
        uint8_t  state;
        uint32_t req_id;

        char friendly_name[256];
        char service_url[256];
        char url_buffer[256];
        char cmd_buffer[256];
        char* liveview_url;

        char* tbl_shutterspd;
        char* tbl_iso;

        char     rx_buff[SHCAM_RXBUFF_SIZE + 1];
        uint32_t rx_buff_idx;

        AsyncHTTPRequest* httpreq = NULL;
        static int read_in_chunk(AsyncHTTPRequest* req, int32_t chunk, char* buff, uint32_t* buff_idx);

        uint32_t dd_tries;
        uint32_t error_cnt;
        uint8_t  event_api_version;

        uint32_t last_poll_time;
        uint32_t poll_delay;

        char str_shutterspd[32];
        char str_shutterspd_clean[32];
        char str_iso[32];
        char str_focusstatus[32];
        char str_afmode[32];

        int8_t   zoom_state;
        uint32_t zoom_time;
        bool     is_movierecording_v;
        bool     is_manuallyfocused_v;

        bool parse_event(char* data, int32_t maxlen = 0);
        void parse_dd_xml(char* data, int32_t maxlen = 0);
        void get_event(void);
        void get_dd_xml(void);
        uint32_t event_found_flag;

        void cmd_prep(void);
        void request_prep(void);
        void request_close(void);

        static DebuggingSerial* dbgser_important;
        static DebuggingSerial* dbgser_states;
        static DebuggingSerial* dbgser_events;
        static DebuggingSerial* dbgser_rx;
        static DebuggingSerial* dbgser_tx;
        static DebuggingSerial* dbgser_devprop_dump;
        static DebuggingSerial* dbgser_devprop_change;

    private:
        static void ddRequestCb      (void* optParm, AsyncHTTPRequest* request, int readyState);
        static void eventRequestCb   (void* optParm, AsyncHTTPRequest* request, int readyState);
        static void initRequestCb    (void* optParm, AsyncHTTPRequest* request, int readyState);
        static void genericRequestCb (void* optParm, AsyncHTTPRequest* request, int readyState);
        static void eventDataCb      (void* optParm, AsyncHTTPRequest* req, int avail);
    public:
        inline char*    get_shutterspd_str  (void) { return str_shutterspd_clean; };
        inline uint32_t get_shutterspd_32   (void) { return parse_shutter_speed_str(str_shutterspd_clean); };
        inline int      get_shutterspd_idx  (void) { return (tbl_shutterspd != NULL) ? get_idx_within_strtbl(tbl_shutterspd, str_shutterspd) : -1; };
        inline char*    get_iso_str         (void) { return str_iso; };
        inline int      get_iso_idx         (void) { return (tbl_iso != NULL) ? get_idx_within_strtbl(tbl_iso, str_iso) : -1; };

        uint32_t get_another_shutterspd(int idx, char* tgt);

        void cmd_Shoot(void);
        void cmd_MovieRecord(bool is_start);
        void cmd_MovieRecordToggle(void);
        void cmd_ZoomStart(int dir);
        void cmd_ZoomStop(void);
        void cmd_FocusPointSet16(int16_t x, int16_t y);
        void cmd_FocusPointSetF(float x, float y);
        void cmd_ShutterSpeedSetStr(char*);
        void cmd_IsoSet(uint32_t x);
        void cmd_IsoSetStr(char*);
        void cmd_ManualFocusMode(bool onoff, bool precheck = false);
        void cmd_ManualFocusToggle(bool onoff);
        void cmd_AutoFocus(bool onoff);
};

#endif
