#ifndef _PTPIPCAMERA_H_
#define _PTPIPCAMERA_H_

#include <stdint.h>
#include <stdbool.h>
#include <WiFi.h>
#include "esp_wifi.h"

#include <DebuggingSerial.h>

#include "ptpipdefs.h"
#include "ptpcodes.h"

enum
{
    PTPSTATE_INIT         = 0,
    PTPSTATE_START_WAIT   = 2,
    PTPSTATE_SOCK_CONN    = 4,
    PTPSTATE_CMD_REQ      = 6,
    PTPSTATE_EVENT_REQ    = 8,
    PTPSTATE_OPENSESSION  = 10,
    PTPSTATE_SESSION_INIT = 12,
    PTPSTATE_POLLING      = 0x0100,
    PTPSTATE_DISCONNECT   = 0x1000,
    PTPSTATE_DISCONNECTED = 0x1002,
};

enum
{
    PTPSTREAMSTATE_NONE,
    PTPSTREAMSTATE_START,
    PTPSTREAMSTATE_GOING,
    PTPSTREAMSTATE_DONE_EMPTY,
};

#define PACKET_BUFFER_SIZE (1024 * 6) // needs to be just big enough for a whole device properties packet
#define DATA_BUFFER_SIZE   (1024 * 6) // needs to be just big enough for a whole device properties packet
#define NAME_BUFFER_SIZE    256

//#define PTPIP_KEEP_STATS
#define PTPIP_IGNORE_INIT_ERROR

#define DEFAULT_BUSY_TIMEOUT 1000

#define PTP_GUID_LEN 16

//#define USE_ASYNC_SOCK
#ifdef USE_ASYNC_SOCK
#include <AsyncTCP.h>
#endif

typedef struct
{
    const uint32_t op_code;
    const uint32_t params[3];
    const uint8_t  params_cnt;
}
ptpip_init_substep_t;

typedef struct
{
    uint16_t prop_code;
    uint16_t data_type;
    int32_t  value;
}
ptpipcam_prop_t;

enum
{
    PTPDEBUGFLAG_NONE   = 0x00,
    PTPDEBUGFLAG_STATES = 0x01,
    PTPDEBUGFLAG_EVENTS = 0x02,
    PTPDEBUGFLAG_RX     = 0x04,
    PTPDEBUGFLAG_TX     = 0x08,
    PTPDEBUGFLAG_DEVPROP_DUMP   = 0x10,
    PTPDEBUGFLAG_DEVPROP_CHANGE = 0x20,
};

class PtpIpCamera
{
    public:
        PtpIpCamera(char* name);
        void            begin(uint32_t ip, uint32_t wait = 1000);
        virtual void    task(void);
        void            poll(void);
        inline bool     isAlive(void)         { return (state & 0xF000) == 0; };
        inline bool     canSend(void)         { return (state & 0x0001) == 0
                                                #ifdef USE_ASYNC_SOCK
                                                && socket_main.canSend()
                                                #endif
                                                    ; };
        inline bool     isOperating(void)     { return (state & 0x0F00) != 0; };
        inline int      getState(void)        { return state; };
        inline uint32_t getIp(void)           { return ip_addr; };
        inline uint32_t getLastRxTime(void)   { return last_rx_time; };
        inline char*    getCameraName(void)   { return (char*)cam_name; };
        inline bool     canNewConnect(void)   { return state < PTPSTATE_START_WAIT || state >= PTPSTATE_DISCONNECTED; };
               bool     isKindaBusy(void);
               bool     isPairingWaiting(void);
        bool send_oper_req(uint32_t opcode, uint32_t* params, uint8_t params_cnt, uint8_t* payload, int32_t payload_len);
        void wait_while_busy(uint32_t min_wait, uint32_t max_wait, volatile bool* exit_signal = NULL);

        inline char* donateBuffer(void) { return (char*)databuff; };
        void generate_guid(char*);
        void install_guid(char*);
        void force_disconnect(void);

        void (*cb_onConnect)(void) = NULL;
        void (*cb_onCriticalError)(void) = NULL;
        void (*cb_onDisconnect)(void) = NULL;
        void (*cb_onReject)(void) = NULL;
        void (*cb_onConfirmedAvail)(void) = NULL;
        void (*cb_onEvent)(uint16_t) = NULL;
        void (*cb_onRxAct)(void) = NULL;

        virtual void set_debugflags(uint32_t x);
        uint32_t debug_flags;
        void test_debug_msg(const char*);

#ifdef PTPIP_KEEP_STATS
        uint32_t stats_tx;
        uint32_t stats_acks;
        uint32_t stats_pkts;
#endif

        uint8_t critical_error_cnt;

    protected:
        int state;
        int substate;
        uint32_t ip_addr;
        bool need_disconnect = false;
        #ifndef USE_ASYNC_SOCK
        WiFiClient socket_main;
        WiFiClient socket_event;
        #else
        AsyncClient socket_main;
        AsyncClient socket_event;
        #endif
        void poll_socket (
            #ifndef USE_ASYNC_SOCK
                WiFiClient* sock
            #else
                AsyncClient* sock
            #endif
                , uint8_t buff[], uint32_t* buff_idx, uint32_t buff_max
            #ifdef USE_ASYNC_SOCK
                , struct pbuf* pb
            #endif
                );
        virtual bool decode_pkt    (uint8_t buff[], uint32_t buff_len);
        bool         try_decode_pkt(uint8_t buff[], uint32_t* buff_idx, uint32_t buff_max, bool can_force);
        virtual bool check_name    (void);
        void         fill_guid     (char*);

        uint32_t last_rx_time;
        uint32_t pending_data; // technically this should be 64 bits
        int32_t incomplete_time;
        uint32_t conn_wait;

        // buffers
        uint32_t pktbuff_idx;
        uint32_t databuff_idx;
        uint32_t eventbuff_idx;
        uint8_t  pktbuff  [PACKET_BUFFER_SIZE];
        uint8_t  databuff [DATA_BUFFER_SIZE];
        uint8_t  eventbuff[PACKET_BUFFER_SIZE];
        uint8_t  outbuff  [PACKET_BUFFER_SIZE];
        void     reset_buffers(void);

        // the protocol requires that these must be kept for the session
        uint32_t conn_id;
        uint32_t session_id;
        uint32_t transaction_id;
        void     parse_cmd_ack(uint8_t*);

        uint8_t error_cnt;

        // name strings
        char my_name [NAME_BUFFER_SIZE];
        char cam_name[NAME_BUFFER_SIZE];
        char* custom_guid = NULL;

        // derived class must fill this
        ptpip_init_substep_t* init_substeps;
        uint8_t init_substeps_cnt;

        // data sending functions
        bool send_data(uint8_t* payload, uint32_t payload_len);
        bool send_cmd_req(void);
        bool send_event_req(void);
        bool send_probe_resp(void);
        bool send_open_session(void);
        void send_debug(char* s);
        void debug_rx(uint8_t*, uint32_t);

        void (*cb_stream)(uint8_t* buff, uint32_t len) = NULL;
        void (*cb_stream_done)(void) = NULL;
        void start_stream(void (*cb_s)(uint8_t*, uint32_t), void (*cb_d)(void));
        uint8_t stream_state = PTPSTREAMSTATE_NONE;

        #ifdef USE_ASYNC_SOCK
        void        wait_canSend      (AsyncClient* sock, uint32_t max_time);
        static void onAsyncPacket     (void*, AsyncClient*, struct pbuf *pb);
        static void onAsyncPacketEvent(void*, AsyncClient*, struct pbuf *pb);
        static void onAsyncError      (void*, AsyncClient*, int8_t);
        static void onAsyncTimeout    (void*, AsyncClient*, uint32_t);
        static void onAsyncAck        (void*, AsyncClient*, size_t, uint32_t);
        #endif

        DebuggingSerial* dbgser_important;
        DebuggingSerial* dbgser_states;
        DebuggingSerial* dbgser_events;
        DebuggingSerial* dbgser_rx;
        DebuggingSerial* dbgser_tx;
        DebuggingSerial* dbgser_devprop_dump;
        DebuggingSerial* dbgser_devprop_change;
};

#endif
