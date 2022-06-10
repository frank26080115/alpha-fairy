#include "PtpIpCamera.h"
#include <Arduino.h>
#include "ptpip_utils.h"

#define PTPIP_TIMEOUT 200
#define PTPIP_CONN_TIMEOUT 5000
#define PTPIP_PACKET_TIMEOUT 2000
#define PTPIP_ERROR_THRESH 10

#define PTPSTATE_ERROR_PRINTF Serial.printf
#if 1
    #define PTPSTATE_PRINTF Serial.printf
#else
    #define PTPSTATE_PRINTF(...)
#endif
#if 1
    #define DATARX_PRINTF Serial.printf
#else
    #define DATARX_PRINTF(...)
#endif
#if 1
    #define EVENT_PRINTF Serial.printf
#else
    #define EVENT_PRINTF(...)
#endif

PtpIpCamera::PtpIpCamera(char* name) {
    strcpy(my_name, name);
    state = PTPSTATE_INIT;
}

void PtpIpCamera::begin(uint32_t ip) {
    PTPSTATE_PRINTF("PTP camera beginning connection %08X\r\n", ip);
    reset_buffers();
    error_cnt = 0;
    socket_main.connect (IPAddress(ip), PTP_OVER_IP_PORT, PTPIP_TIMEOUT);
    socket_event.connect(IPAddress(ip), PTP_OVER_IP_PORT, PTPIP_TIMEOUT);
    state = PTPSTATE_SOCK_CONN;
    last_rx_time = millis();
}

void PtpIpCamera::task()
{
    uint32_t now = millis();
    if (state == PTPSTATE_INIT) {
        return;
    }
    if (state == PTPSTATE_SOCK_CONN) {
        if (socket_main.connected() && socket_event.connected()) {
            last_rx_time = now;
            state += 2;
            PTPSTATE_PRINTF("PTP sockets connected\r\n");
            return;
        }
        else if ((now - last_rx_time) > PTPIP_CONN_TIMEOUT) {
            PTPSTATE_ERROR_PRINTF("PTP connection timed out\r\n");
            state = PTPSTATE_DISCONNECTED;
            return;
        }
    }
    if (state >= PTPSTATE_DISCONNECTED && (state & 1) == 0) {
        socket_main.stop();
        socket_event.stop();
        PTPSTATE_ERROR_PRINTF("PTP socket stopping due to disconnection\r\n");
        state |= 1;
        return;
    }
    if (state < PTPSTATE_DISCONNECTED && error_cnt >= PTPIP_ERROR_THRESH) {
        PTPSTATE_ERROR_PRINTF("PTP socket too many errors (state %u)\r\n", state);
        state = PTPSTATE_DISCONNECTED;
        return;
    }

    if (state > PTPSTATE_SOCK_CONN + 1 && state < PTPSTATE_DISCONNECTED)
    {
        if (socket_main.connected() == 0 || socket_event.connected() == 0) {
            PTPSTATE_ERROR_PRINTF("PTP socket disconnected (state %u)\r\n", state);
            state = PTPSTATE_DISCONNECTED;
            return;
        }
        poll();
        //now = millis();
        if (now > last_rx_time && (now - last_rx_time) > PTPIP_PACKET_TIMEOUT) {
            if (pktbuff_idx > 0) {
                PTPSTATE_ERROR_PRINTF("PTP timeout receiving packet (%u rem)\r\n", pktbuff_idx);
            }
            reset_buffers();
        }
    }

    if (state == PTPSTATE_CMD_REQ) {
        if (send_cmd_req()) {
            PTPSTATE_PRINTF("PTP init sent CMD_REQ\r\n");
        }
        else {
            PTPSTATE_ERROR_PRINTF("PTP init send error, CMD_REQ failed\r\n");
        }
    }
    else if (state == PTPSTATE_EVENT_REQ) {
        if (send_event_req()) {
            PTPSTATE_PRINTF("PTP init sent EVENT_REQ\r\n");
        }
        else {
            PTPSTATE_ERROR_PRINTF("PTP init send error, EVENT_REQ failed\r\n");
        }
    }
    else if (state == PTPSTATE_OPENSESSION) {
        if (send_open_session()) {
            PTPSTATE_PRINTF("PTP init sent open_session\r\n");
        }
        else {
            PTPSTATE_ERROR_PRINTF("PTP init send error, open_session failed\r\n");
        }
    }
    else if (state >= PTPSTATE_SESSION_INIT && canSend()) {
        if (init_substeps == NULL)
        {
            PTPSTATE_PRINTF("PTP init no other tasks, now polling\r\n");
            state = PTPSTATE_POLLING;
        }
        else
        {
            ptpip_init_substep_t* substepstruct = &(init_substeps[substate]);
            if (substepstruct->op_code != 0)
            {
                if (send_oper_req(substepstruct->op_code, (uint32_t*)(substepstruct->params), substepstruct->params_cnt, NULL, -1)) {
                    PTPSTATE_PRINTF("PTP init sent substate %u\r\n", substate);
                }
                else {
                    PTPSTATE_ERROR_PRINTF("PTP init failed to send substate %u\r\n", substate);
                }
            }
            else // 0 in the table's opcode means end-of-table
            {
                PTPSTATE_PRINTF("PTP init finished sending all substates %u\r\n", substate);
                PTPSTATE_PRINTF("PTP init done, now polling\r\n");
                state = PTPSTATE_POLLING;
            }
        }
    }
}

void PtpIpCamera::poll()
{
    poll_socket(&socket_main , pktbuff  , &pktbuff_idx  , PACKET_BUFFER_SIZE);
    poll_socket(&socket_event, eventbuff, &eventbuff_idx, PACKET_BUFFER_SIZE);
}

void PtpIpCamera::poll_socket(WiFiClient* sock, uint8_t buff[], uint32_t* buff_idx, uint32_t buff_max)
{
    uint32_t now = millis();
    int avail, read_limit, to_read, did_read;
    if (sock->connected() == false) {
        error_cnt += 1;
        return;
    }
    if ((avail = sock->available()) <= 0) {
        return;
    }
    read_limit = buff_max - (*buff_idx) - 1;
    to_read = avail > read_limit ? read_limit : avail;
    did_read = sock->read((uint8_t*)buff, (size_t)to_read);
    (*buff_idx) += did_read;
    if (did_read > 0) {
        error_cnt = 0;
    }
    last_rx_time = now;
    int retries = 3;
    // the data we just read might contain multiple valid packets
    // we try to decode as much of it as possible
    do
    {
        if ((*buff_idx) > 8) {
            if (try_decode_pkt(buff, buff_idx, buff_max) == false) {
                break;
            }
        }
    }
    while ((retries--) > 0);
}

bool PtpIpCamera::try_decode_pkt(uint8_t buff[], uint32_t* buff_idx, uint32_t buff_max)
{
    bool did_stuff = false;
    ptpip_pkthdr_t* hdr = (ptpip_pkthdr_t*)buff;

    if ((*buff_idx) >= hdr->length)
    {
        if (pending_data <= 0)
        {
            decode_pkt(buff, *buff_idx);
            buffer_consume(buff, buff_idx, hdr->length, buff_max);
            did_stuff = true;
        }
        else // pending_data > 0
        {
            if (hdr->pkt_type == PTP_PKTTYPE_DATA)
            {
                uint32_t chunk_size = hdr->length - 12;
                uint32_t copy_size = chunk_size;
                if ((databuff_idx + copy_size) >= DATA_BUFFER_SIZE) {
                    copy_size = DATA_BUFFER_SIZE - databuff_idx - 1;
                }
                memcpy(&(databuff[databuff_idx]), &(buff[12]), copy_size);
                databuff_idx += copy_size;
                pending_data -= hdr->length;
                buffer_consume(buff, buff_idx, chunk_size, buff_max);
                did_stuff = true;
            }
            else if (hdr->pkt_type == PTP_PKTTYPE_ENDDATA || hdr->pkt_type == PTP_PKTTYPE_CANCELDATA)
            {
                pending_data = 0;
                did_stuff = try_decode_pkt(buff, buff_idx, buff_max); // no chance of infinite recursion due to pending_data = 0
            }
            else
            {
                decode_pkt(buff, *buff_idx);
                buffer_consume(buff, buff_idx, hdr->length, buff_max);
                did_stuff = true;
            }
        }
    }
    else if ((*buff_idx) >= 8)
    {
        // special case for incomplete packet, I see this come consistently from my camera
        decode_pkt(buff, *buff_idx);
        buffer_consume(buff, buff_idx, (*buff_idx), buff_max);
        did_stuff = true;
    }
    return did_stuff;
}

void PtpIpCamera::decode_pkt(uint8_t buff[], uint32_t buff_len)
{
    ptpip_pkthdr_t* hdr = (ptpip_pkthdr_t*)buff;
    uint32_t pkt_len = hdr->length;
    uint32_t pkt_type = hdr->pkt_type;
    if (pkt_type == PTP_PKTTYPE_INITCMDACK && state <= (PTPSTATE_CMD_REQ + 1))
    {
        parse_cmd_ack(buff);
        state = PTPSTATE_EVENT_REQ;
        PTPSTATE_PRINTF("PTP init next state EVENT_REQ (%u)\r\n", state);
    }
    else if (pkt_type == PTP_PKTTYPE_INITEVENTACK && state <= (PTPSTATE_EVENT_REQ + 1))
    {
        state = PTPSTATE_OPENSESSION;
        PTPSTATE_PRINTF("PTP init next state OPEN_SESSION (%u)\r\n", state);
    }
    else if (pkt_type == PTP_PKTTYPE_OPERRESP && state <= (PTPSTATE_OPENSESSION + 1))
    {
        ptpip_pkt_operresp_t* operresp = (ptpip_pkt_operresp_t*)buff;
        if (PTP_RESPCODE_IS_OK_ISH(operresp->resp_code)
                || buff_len == 8 // odd case for incomplete packet, assume a success response code
            ) {
            state = PTPSTATE_SESSION_INIT;
            substate = 0;
            PTPSTATE_PRINTF("PTP init, session opened %u, next state SESSION_INIT (%u)\r\n", session_id, state);
        }
        else {
            PTPSTATE_ERROR_PRINTF("PTP init OPEN_SESSION failed 0x%04X\r\n", operresp->resp_code);
        }
    }
    else if (pkt_type == PTP_PKTTYPE_OPERRESP && state >= PTPSTATE_SESSION_INIT && state < PTPSTATE_POLLING)
    {
        ptpip_pkt_operresp_t* operresp = (ptpip_pkt_operresp_t*)buff;
        if (PTP_RESPCODE_IS_OK_ISH(operresp->resp_code) || buff_len == 8) {
            state += 1;
            substate += 1;
            PTPSTATE_PRINTF("PTP init next state/substate %u/%u\r\n", state, substate);
        }
        else {
            PTPSTATE_ERROR_PRINTF("PTP init oper-resp failed 0x%04X (state/substate %u/%u)\r\n", operresp->resp_code, state, substate);
        }
        state &= 0xFFFFFFFE;
    }
    else if (pkt_type == PTP_PKTTYPE_OPERRESP && state >= PTPSTATE_POLLING)
    {
        state &= 0xFFFFFFFE;
    }
    else if (pkt_type == PTP_PKTTYPE_STARTDATA)
    {
        ptpip_pkt_startdata_t* pktstruct = (ptpip_pkt_startdata_t*)buff;
        pending_data = pktstruct->pending_data_length;
        databuff_idx = 0;
        DATARX_PRINTF("PTPIP start-data %u\r\n", pending_data);
    }
    else if (pkt_type == PTP_PKTTYPE_ENDDATA)
    {
        DATARX_PRINTF("PTPIP end-data\r\n");
    }
    else if (pkt_type == PTP_PKTTYPE_CANCELDATA)
    {
        DATARX_PRINTF("PTPIP cancel-data\r\n");
    }
    else if (pkt_type == PTP_PKTTYPE_DATA)
    {
        DATARX_PRINTF("PTPIP data chunk %u\r\n", pkt_len);
    }
    else if (pkt_type == PTP_PKTTYPE_EVENT)
    {
        ptpip_pkt_event_t* pktstruct = (ptpip_pkt_event_t*)buff;
        uint16_t event_code = pktstruct->event_code;
        EVENT_PRINTF("PTPIP event 0x%04X\r\n", event_code);
    }
    else if (pkt_type == PTP_PKTTYPE_PROBEREQ)
    {
        send_probe_resp();
        EVENT_PRINTF("PTPIP probe request\r\n");
    }
    else
    {
        PTPSTATE_ERROR_PRINTF("PTP unknown packet type 0x%08X\r\n", pkt_type);
    }
}

void PtpIpCamera::parse_cmd_ack(uint8_t* data)
{
    ptpip_pkt_cmdack_t* pktstruct = (ptpip_pkt_cmdack_t*)data;
    conn_id = pktstruct->conn_id;
    copy_utf16_to_bytes(pktstruct->name, cam_name);
    PTPSTATE_PRINTF("PTP recv'ed CMD-ACK, conn-ID 0x%08X, name: %s\r\n", conn_id, cam_name);
}

void PtpIpCamera::wait_while_busy(uint32_t min_time, uint32_t max_time, volatile bool* exit_signal)
{
    volatile bool to_exit = false;
    uint32_t start_time = millis();
    uint32_t now = start_time;
    while ((canSend() == false && (now - start_time) < max_time) || (now - start_time) < min_time) {
        now = millis();
        poll(); // poll, not task, because poll only reads and never sends
        if (exit_signal != NULL)
        {
            to_exit |= *exit_signal;
            if (to_exit && (now - start_time) >= min_time) {
                break;
            }
        }
    }
}

void PtpIpCamera::reset_buffers()
{
    pktbuff_idx = 0;
    eventbuff_idx = 0;
    databuff_idx = 0;
}
