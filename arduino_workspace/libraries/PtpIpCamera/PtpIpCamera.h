#ifndef _PTPIPCAMERA_H_
#define _PTPIPCAMERA_H_

#include <stdint.h>
#include <stdbool.h>
#include <WiFi.h>
#include "esp_wifi.h"

#include "ptpipdefs.h"
#include "ptpcodes.h"

enum
{
    PTPSTATE_INIT         = 0,
    PTPSTATE_SOCK_CONN    = 2,
    PTPSTATE_CMD_REQ      = 4,
    PTPSTATE_EVENT_REQ    = 6,
    PTPSTATE_OPENSESSION  = 8,
    PTPSTATE_SESSION_INIT = 10,
    PTPSTATE_POLLING      = 0x0100,
    PTPSTATE_DISCONNECTED = 0x1000,
};

#define PACKET_BUFFER_SIZE  1024
#define DATA_BUFFER_SIZE   (1024 * 6) // needs to be just big enough for a whole device properties packet
#define NAME_BUFFER_SIZE    256

#define DEFAULT_BUSY_TIMEOUT 1000

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

class PtpIpCamera
{
    public:
        PtpIpCamera(char* name);
        void            begin(uint32_t);
        virtual void    task(void);
        void            poll(void);
        inline bool     isAlive(void)         { return (state & 0xF000) == 0; };
        inline bool     canSend(void)         { return (state & 0x0001) != 0; };
        inline bool     isOperating(void)     { return (state & 0x0F00) != 0; };
        inline int      getState(void)        { return state; };
        inline uint32_t getLastRxTime(void)   { return last_rx_time; };
        inline char*    getCameraName(void)   { return (char*)cam_name; };
        bool send_oper_req(uint32_t opcode, uint32_t* params, uint8_t params_cnt, uint8_t* payload, int32_t payload_len);
        void wait_while_busy(uint32_t min_wait, uint32_t max_wait, volatile bool* exit_signal);
    protected:
        int state;
        int substate;
        WiFiClient socket_main;
        WiFiClient socket_event;
        virtual void decode_pkt    (uint8_t buff[], uint32_t buff_len);
        bool         try_decode_pkt(uint8_t buff[], uint32_t* buff_idx, uint32_t buff_max);
        void         poll_socket   (WiFiClient* sock, uint8_t buff[], uint32_t* buff_idx, uint32_t buff_max);

        uint32_t last_rx_time;
        uint64_t pending_data;

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
};

#endif
