#ifndef _SONYHTTPCAMERA_H_
#define _SONYHTTPCAMERA_H_

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <string.h>

#include <AsyncHTTPRequest_Generic.h> 

#define SHCAM_RXBUFF_UNIT     64
#define SHCAM_RXBUFF_UNIT_CNT (4 * 4)
#define SHCAM_RXBUFF_SIZE     (SHCAM_RXBUFF_UNIT * SHCAM_RXBUFF_UNIT_CNT)

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

class SonyHttpCamera
{
    public:
        SonyHttpCamera();
        void begin(uint32_t ip);
        void poll(void);

        inline uint32_t  getIp           (void) { return ip_addr; };
        inline char*     getCameraName   (void) { return friendly_name; };
        inline uint8_t   getState        (void) { return state; };
        inline bool      canSend         (void) { return state >= SHCAMSTATE_READY && (state & 1) == 0; };
        inline bool      isOperating     (void) { return state >= SHCAMSTATE_READY && state < SHCAMSTATE_FAILED; };
        inline bool      canNewConnect   (void) { return state == SHCAMSTATE_NONE || state == SHCAMSTATE_FAILED || state == SHCAMSTATE_DISCONNECTED };
        inline void      setForbidden    (void) { state = SHCAMSTATE_FORBIDDEN; }

        inline int       getPollDelay    (void)       { return poll_delay; };
        inline void      setPollDelay    (uint32_t x) { poll_delay = x; };

        bool is_movierecording;

        void disconnect(void) { state = SHCAMSTATE_DISCONNECTED; };
        void wait_while_busy(uint32_t min_wait, uint32_t max_wait, volatile bool* exit_signal = NULL);

        void (*cb_onConnect)(void) = NULL;
        //void (*cb_onCriticalError)(void) = NULL;
        void (*cb_onDisconnect)(void) = NULL;
        //void (*cb_onReject)(void) = NULL;

    protected:
        uint32_t ip_addr;
        uint8_t  state;

        char friendly_name[256];
        char service_url[256];
        char url_buffer[256];

        char     rx_buff[SHCAM_RXBUFF_SIZE + 1];
        uint32_t rx_buff_idx;

        AsyncHTTPRequest* httpreq = NULL;

        uint32_t dd_tries;
        uint32_t error_cnt;

        uint32_t poll_delay;

        char str_shutterspd[32];
        char str_shutterspd_clean[32];
        char str_iso[32];

        int8_t   zoom_state;
        uint32_t zoom_time;

        void parse_event(char* data, int32_t maxlen = 0);
        void parse_dd_xml(char* data, int32_t maxlen = 0);
        void get_event(void);
        void get_dd_xml(void);
    private:
        static void ddRequestCb   (void* optParm, AsyncHTTPRequest* request, int readyState);
        static void eventRequestCb(void* optParm, AsyncHTTPRequest* request, int readyState);
        static void initRequestCb (void* optParm, AsyncHTTPRequest* request, int readyState);
        static void eventDataCb   (void* optParm, AsyncHTTPRequest* req, int avail);
    public:
        void cmd_Shoot(void);
        void cmd_MovieRecord(bool is_start);
        void cmd_MovieRecordToggle(void);
        void cmd_ZoomStart(void);
        void cmd_ZoomStop(void);
        void cmd_FocusAreaSet16(int16_t x, int16_t y);
        void cmd_FocusAreaSetF(float x, float y);
        void cmd_ShutterSpeedSetStr(char*);
        void cmd_IsoSet(uint32_t x);
        void cmd_IsoSetStr(char*);
};

void read_in_chunk(AsyncHTTPRequest* req, int32_t chunk, char* buff, int32_t* buff_idx);
bool scan_json_for_key(char* data, int32_t datalen, char* keystr, signed int* start_idx, signed int* end_idx, char* tgt, int tgtlen);
int count_commas(char* data);
void strcpy_no_slash(char* dst, char* src);
bool get_txt_within_strtbl(char* tbl, int idx, char* tgt);
int get_idx_within_strtbl(char* tbl, char* needle);

#endif
