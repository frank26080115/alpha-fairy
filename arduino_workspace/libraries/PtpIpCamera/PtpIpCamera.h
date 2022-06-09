#ifndef _PTPIPCAMERA_H_
#define _PTPIPCAMERA_H_

#include <stdint.h>
#include <stdbool.h>
#include <WiFi.h>
#include <AsyncTCP.h>
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

#define PACKET_BUFFER_SIZE 1024
#define DATA_BUFFER_SIZE 1024
#define NAME_BUFFER_SIZE 256

class PtpIpCamera
{
    public:
        PtpIpCamera(char*);
        void begin(ip4_addr_t);
        void task(void);
        inline bool     isAlive(void)         { return (_state & 0xF000) == 0; };
        inline bool     canSend(void)         { return (_state & 0x0001) != 0; };
        inline bool     isOperating(void)     { return (_state & 0x0F00) != 0; };
        inline int      getState(void)        { return _state; };
        inline uint32_t getLastActTime(void)  { return _last_act_time; };
        inline char*    getCameraName(void)   { return (char*)cam_name; };
        void send_opreq(uint32_t opcode, uint32_t* params, uint8_t params_cnt, uint8_t* payload, uint32_t payload_len, bool block);
        virtual void wait_while_busy(uint32_t min_wait, uint32_t max_wait);
    protected:
        int _state;
        int _substate;
        AsyncClient* _socket_main;
        AsyncClient* _socket_event;
        void send_data(uint8_t*, uint32_t);
        virtual void stream(uint8_t* data, uint32_t data_len) { };
        virtual void decode(uint8_t* data, uint32_t data_len);
        uint32_t _last_act_time;
        uint32_t _last_rx_time;
        uint64_t _pending_data;
        uint32_t _pending_data_chunk;
        uint32_t _pktbuff_idx;
        uint32_t _databuff_idx;
        uint32_t _eventbuff_idx;
        uint8_t  pktbuff  [PACKET_BUFFER_SIZE];
        uint8_t  databuff [DATA_BUFFER_SIZE];
        uint8_t  eventbuff[PACKET_BUFFER_SIZE];
        uint8_t  outbuff  [PACKET_BUFFER_SIZE];
        uint32_t conn_id;
        uint32_t session_id;
        uint32_t transaction_id;
        char     my_name [NAME_BUFFER_SIZE];
        char     cam_name[NAME_BUFFER_SIZE];
        void send_cmd_req(void);
        void decode_cmd_ack(void);
        void send_event_req(void);
        void send_open_session(void);
        void send_probe_response(void);
};

#endif
