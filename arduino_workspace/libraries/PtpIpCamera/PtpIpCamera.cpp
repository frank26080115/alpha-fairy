#include "PtpIpCamera.h"
#include <Arduino.h>
#include "ptpip_utils.h"

#define PTPIP_TIMEOUT 5000
#define PTPIP_CONN_TIMEOUT 5000
#define PTPIP_CONN_WAIT 1000
#define PTPIP_PACKET_TIMEOUT 5000
#define PTPIP_ERROR_THRESH 10

//#define PTPIP_DEBUG_RX

PtpIpCamera::PtpIpCamera(char* name) {
    strcpy(my_name, name);
    state = PTPSTATE_INIT;
    #ifdef PTPIP_KEEP_STATS
    stats_pkts = 0;
    stats_acks = 0;
    stats_tx = 0;
    #endif

    dbgser_important       = new DebuggingSerial(&Serial);
    dbgser_states          = new DebuggingSerial(&Serial);
    dbgser_events          = new DebuggingSerial(&Serial);
    dbgser_rx              = new DebuggingSerial(&Serial);
    dbgser_tx              = new DebuggingSerial(&Serial);
    dbgser_devprop_dump    = new DebuggingSerial(&Serial);
    dbgser_devprop_change  = new DebuggingSerial(&Serial);

    dbgser_important->enabled = true;
    dbgser_states->   enabled = true;
    dbgser_events->   enabled = true;
    dbgser_rx->       enabled = false;
    #ifdef PTPIP_DEBUG_RX
    dbgser_rx->enabled = true;
    #endif
    dbgser_tx->enabled = false;
    dbgser_devprop_dump->  enabled = false;
    dbgser_devprop_change->enabled = true;
}

void PtpIpCamera::begin(uint32_t ip) {
    if (ip == 0) {
        //dbgser_important->printf("PTP camera got an empty IP address\r\n");
        return;
    }
    if (state > PTPSTATE_START_WAIT && state < PTPSTATE_DISCONNECTED) {
        return;
    }

    dbgser_states->printf("PTP camera beginning connection %08X\r\n", ip);
    ip_addr = ip;
    state = PTPSTATE_START_WAIT;
    last_rx_time = millis();
}

void PtpIpCamera::task()
{
    uint32_t now = millis();
    if (state == PTPSTATE_INIT) {
        return;
    }
    if (state == PTPSTATE_START_WAIT && (now - last_rx_time) > PTPIP_CONN_WAIT) {
        reset_buffers();
        error_cnt = 0;

        #ifdef USE_ASYNC_SOCK
        socket_main.onPacket  (this->onAsyncPacket     , this);
        socket_main.onError   (this->onAsyncError      , this);
        socket_main.onTimeout (this->onAsyncTimeout    , this);
        socket_main.onAck     (this->onAsyncAck        , this);
        socket_event.onPacket (this->onAsyncPacketEvent, this);
        socket_event.onError  (this->onAsyncError      , this);
        socket_event.onTimeout(this->onAsyncTimeout    , this);
        socket_event.onAck    (this->onAsyncAck        , this);
        #endif

        socket_main.connect (IPAddress(ip_addr), PTP_OVER_IP_PORT
            #ifndef USE_ASYNC_SOCK
            , PTPIP_CONN_TIMEOUT
            #endif
            );
        socket_event.connect(IPAddress(ip_addr), PTP_OVER_IP_PORT
            #ifndef USE_ASYNC_SOCK
            , PTPIP_CONN_TIMEOUT
            #endif
            );
        state = PTPSTATE_SOCK_CONN;
        last_rx_time = now;
    }
    if (state == PTPSTATE_SOCK_CONN) {
        if (socket_main.connected() && socket_event.connected()) {
            #ifndef USE_ASYNC_SOCK
                socket_main .setTimeout(PTPIP_TIMEOUT);
                socket_event.setTimeout(PTPIP_TIMEOUT);
            #else
                socket_main .setAckTimeout(PTPIP_TIMEOUT);
                socket_event.setAckTimeout(PTPIP_TIMEOUT);
                //dbgser_states->printf("PTP socket MSS %u\r\n", socket_main.getMss());
            #endif
            last_rx_time = now;
            state += 2;
            dbgser_states->printf("PTP sockets connected\r\n");
            return;
        }
        else if ((now - last_rx_time) > PTPIP_CONN_TIMEOUT) {
            dbgser_important->printf("PTP connection timed out\r\n");
            state = PTPSTATE_DISCONNECTED;
            return;
        }
    }
    if (state == PTPSTATE_DISCONNECT) {
        #ifdef USE_ASYNC_SOCK
        socket_main .close(true);
        socket_event.close(true);
        #else
        socket_main .stop();
        socket_event.stop();
        #endif
        dbgser_important->printf("PTP socket stopping due to disconnection\r\n");
        state |= 1;
        return;
    }
    else if (state == (PTPSTATE_DISCONNECT | 1)) {
        #ifdef USE_ASYNC_SOCK
        if (socket_main.disconnected() && socket_event.disconnected()) {
            state = PTPSTATE_DISCONNECTED;
        }
        #else
        state = PTPSTATE_DISCONNECTED;
        #endif
        return;
    }
    if (state < PTPSTATE_DISCONNECT && error_cnt >= PTPIP_ERROR_THRESH) {
        dbgser_important->printf("PTP socket too many errors (state %u)\r\n", state);
        state = PTPSTATE_DISCONNECT;
        return;
    }

    if (state > PTPSTATE_SOCK_CONN + 1 && state < PTPSTATE_DISCONNECT)
    {
        if (socket_main.connected() == 0 || socket_event.connected() == 0) {
            dbgser_important->printf("PTP socket disconnected (state %u)\r\n", state);
            state = PTPSTATE_DISCONNECT;
            return;
        }
        poll();
        //now = millis();
        uint32_t pkt_timeout = PTPIP_PACKET_TIMEOUT;
        if (pending_data > 0 || pktbuff_idx > 0) {
            pkt_timeout *= 3;
        }
        if (now > last_rx_time && (now - last_rx_time) > pkt_timeout) {
            if (pktbuff_idx > 0) {
                if (try_decode_pkt(pktbuff, &pktbuff_idx, PACKET_BUFFER_SIZE, true) == false) {
                    dbgser_important->printf("PTP timeout receiving packet (%u rem)\r\n", pktbuff_idx);
                }
            }
            last_rx_time = now;
            reset_buffers();
        }
    }

    if (state == PTPSTATE_CMD_REQ) {
        if (send_cmd_req()) {
            dbgser_states->printf("PTP init sent CMD_REQ\r\n");
        }
        else {
            dbgser_important->printf("PTP init send error, CMD_REQ failed\r\n");
        }
    }
    else if (state == PTPSTATE_EVENT_REQ) {
        if (send_event_req()) {
            dbgser_states->printf("PTP init sent EVENT_REQ\r\n");
        }
        else {
            dbgser_important->printf("PTP init send error, EVENT_REQ failed\r\n");
        }
    }
    else if (state == PTPSTATE_OPENSESSION) {
        if (send_open_session()) {
            dbgser_states->printf("PTP init sent open_session\r\n");
        }
        else {
            dbgser_important->printf("PTP init send error, open_session failed\r\n");
        }
    }
    else if (state >= PTPSTATE_SESSION_INIT && state < PTPSTATE_POLLING && canSend()) {
        if (init_substeps == NULL)
        {
            dbgser_states->printf("PTP init no other tasks, now polling\r\n");
            state = PTPSTATE_POLLING;
        }
        else
        {
            ptpip_init_substep_t* substepstruct = &(init_substeps[substate]);
            if (substepstruct->op_code != 0)
            {
                if (send_oper_req(substepstruct->op_code, (uint32_t*)(substepstruct->params), substepstruct->params_cnt, NULL, -1)) {
                    dbgser_states->printf("PTP init sent substate %u\r\n", substate);
                }
                else {
                    dbgser_important->printf("PTP init failed to send substate %u\r\n", substate);
                }
            }
            else // 0 in the table's opcode means end-of-table
            {
                dbgser_states->printf("PTP init finished sending all substates %u\r\n", substate);
                dbgser_states->printf("PTP init done, now polling\r\n");
                state = PTPSTATE_POLLING;
            }
        }
    }
}

void PtpIpCamera::poll()
{
    yield();
    #ifndef USE_ASYNC_SOCK
    poll_socket(&socket_main , pktbuff  , &pktbuff_idx  , PACKET_BUFFER_SIZE);
    poll_socket(&socket_event, eventbuff, &eventbuff_idx, PACKET_BUFFER_SIZE);
    #else
        #ifdef ASYNCTCP_NO_RTOS_TASK
            socket_main.poll_task();
            socket_event.poll_task();
        #endif
    #endif
}

void PtpIpCamera::poll_socket(
    #ifndef USE_ASYNC_SOCK
    WiFiClient* sock,
    #else
    AsyncClient* sock,
    #endif
    uint8_t buff[], uint32_t* buff_idx, uint32_t buff_max
    #ifdef USE_ASYNC_SOCK
    , struct pbuf *pb
    #endif
    )
{
    uint32_t now = millis();
    int avail, read_limit, to_read, did_read;
    #ifndef USE_ASYNC_SOCK
    if (sock->connected() == false) {
        error_cnt += 1;
        return;
    }
    #endif

    #ifndef USE_ASYNC_SOCK
    if ((avail = sock->available()) <= 0) {
        return;
    }
    digitalWrite(10, LOW);
    #else
    avail = pb->len; // note: the _recv function from AsyncClient already handles the pb->next packet for you, do not attempt to chain them here
    #endif
    read_limit = buff_max - (*buff_idx) - 1;
    to_read = avail > read_limit ? read_limit : avail;
    #ifndef USE_ASYNC_SOCK
    did_read = sock->read((uint8_t*)&(buff[*buff_idx]), (size_t)to_read);
    #else
    memcpy(&(buff[*buff_idx]), pb->payload, to_read);
    did_read = to_read;
    #endif
    (*buff_idx) += did_read;

    if (did_read > 0) {
        #ifdef PTPIP_DEBUG_RX
        debug_rx((uint8_t*)buff, did_read);
        #endif
        error_cnt = 0;
    }
    last_rx_time = now;

    int retries = 6;
    // the data we just read might contain multiple valid packets
    // we try to decode as much of it as possible
    do
    {
        yield();
        if ((*buff_idx) >= 8) {
            if (try_decode_pkt(buff, buff_idx, buff_max, false) == false) {
                break;
            }
        }
    }
    while ((retries--) > 0);

    digitalWrite(10, HIGH);
}

bool PtpIpCamera::try_decode_pkt(uint8_t buff[], uint32_t* buff_idx, uint32_t buff_max, bool can_force)
{
    bool did_stuff = false;
    ptpip_pkthdr_t* hdr = (ptpip_pkthdr_t*)buff;

    if ((*buff_idx) >= hdr->length)
    {
        if (pending_data <= 0)
        {
            if (decode_pkt(buff, *buff_idx)) {
                buffer_consume(buff, buff_idx, hdr->length, buff_max);
            }
            else {
                reset_buffers();
            }
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
                pending_data -= chunk_size;
                decode_pkt(buff, *buff_idx); // this should only do a debug print
                buffer_consume(buff, buff_idx, hdr->length, buff_max);
                did_stuff = true;
            }
            else if (hdr->pkt_type == PTP_PKTTYPE_ENDDATA || hdr->pkt_type == PTP_PKTTYPE_CANCELDATA)
            {
                pending_data = 0;
                did_stuff |= try_decode_pkt(buff, buff_idx, buff_max, false); // no chance of infinite recursion due to pending_data = 0
            }
            else
            {
                if (decode_pkt(buff, *buff_idx)) {
                    buffer_consume(buff, buff_idx, hdr->length, buff_max);
                }
                else {
                    reset_buffers();
                }
                did_stuff = true;
            }
        }
    }
    else if ((*buff_idx) >= 8 && can_force)
    {
        decode_pkt(buff, *buff_idx);
        //buffer_consume(buff, buff_idx, (*buff_idx), buff_max);
        // no need to call buffer_consume as the only place that calls can_force will do a reset_buffer anyways
        did_stuff = true;
    }
    return did_stuff;
}

bool PtpIpCamera::decode_pkt(uint8_t buff[], uint32_t buff_len)
{
    bool pkt_valid = true;
    ptpip_pkthdr_t* hdr = (ptpip_pkthdr_t*)buff;
    uint32_t pkt_len = hdr->length;
    uint32_t pkt_type = hdr->pkt_type;
    if (pkt_type == PTP_PKTTYPE_INITCMDACK && state <= (PTPSTATE_CMD_REQ + 1))
    {
        parse_cmd_ack(buff);
        state = PTPSTATE_EVENT_REQ;
        dbgser_states->printf("PTP init next state EVENT_REQ (%u)\r\n", state);
    }
    else if (pkt_type == PTP_PKTTYPE_INITEVENTACK && state <= (PTPSTATE_EVENT_REQ + 1))
    {
        state = PTPSTATE_OPENSESSION;
        dbgser_states->printf("PTP init next state OPEN_SESSION (%u)\r\n", state);
    }
    else if (pkt_type == PTP_PKTTYPE_OPERRESP && state <= (PTPSTATE_OPENSESSION + 1))
    {
        ptpip_pkt_operresp_t* operresp = (ptpip_pkt_operresp_t*)buff;
        if (PTP_RESPCODE_IS_OK_ISH(operresp->resp_code)
                || buff_len == 8 // odd case for incomplete packet, assume a success response code
            ) {
            state = PTPSTATE_SESSION_INIT;
            substate = 0;
            dbgser_states->printf("PTP init, session opened %u, next state SESSION_INIT (%u)\r\n", session_id, state);
        }
        else {
            dbgser_important->printf("PTP init OPEN_SESSION failed 0x%04X\r\n", operresp->resp_code);
        }
    }
    else if (pkt_type == PTP_PKTTYPE_OPERRESP && state >= PTPSTATE_SESSION_INIT && state < PTPSTATE_POLLING)
    {
        ptpip_pkt_operresp_t* operresp = (ptpip_pkt_operresp_t*)buff;
        if (PTP_RESPCODE_IS_OK_ISH(operresp->resp_code) || buff_len == 8) {
            state += 1;
            substate += 1;
            dbgser_states->printf("PTP init next state/substate %u/%u\r\n", state, substate);
        }
        else {
            dbgser_important->printf("PTP init oper-resp failed 0x%04X (state/substate %u/%u)\r\n", operresp->resp_code, state, substate);
        }
        state &= 0xFFFFFFFE;
    }
    else if (pkt_type == PTP_PKTTYPE_OPERRESP && state >= PTPSTATE_POLLING)
    {
        state &= 0xFFFFFFFE;
        ptpip_pkt_operresp_t* operresp = (ptpip_pkt_operresp_t*)buff;
        dbgser_states->printf("PTP got oper-resp 0x%04X\r\n", operresp->resp_code);
    }
    else if (pkt_type == PTP_PKTTYPE_STARTDATA)
    {
        ptpip_pkt_startdata_t* pktstruct = (ptpip_pkt_startdata_t*)buff;
        pending_data = pktstruct->pending_data_length;
        databuff_idx = 0;
        dbgser_rx->printf("PTPRX start-data %u\r\n", pending_data);
    }
    else if (pkt_type == PTP_PKTTYPE_ENDDATA)
    {
        dbgser_rx->printf("PTPRX end-data\r\n");
    }
    else if (pkt_type == PTP_PKTTYPE_CANCELDATA)
    {
        dbgser_rx->printf("PTPRX cancel-data\r\n");
    }
    else if (pkt_type == PTP_PKTTYPE_DATA)
    {
        dbgser_rx->printf("PTPRX data chunk %u\r\n", (pkt_len - 12));
    }
    else if (pkt_type == PTP_PKTTYPE_EVENT)
    {
        ptpip_pkt_event_t* pktstruct = (ptpip_pkt_event_t*)buff;
        uint16_t event_code = pktstruct->event_code;
        dbgser_events->printf("PTPIP event 0x%04X\r\n", event_code);
    }
    else if (pkt_type == PTP_PKTTYPE_PROBEREQ)
    {
        send_probe_resp();
        dbgser_events->printf("PTPIP probe request\r\n");
    }
    else
    {
        dbgser_important->printf("PTP unknown packet type 0x%08X\r\n", pkt_type);
        pkt_valid = false;
    }
    return pkt_valid;
}

void PtpIpCamera::parse_cmd_ack(uint8_t* data)
{
    ptpip_pkt_cmdack_t* pktstruct = (ptpip_pkt_cmdack_t*)data;
    conn_id = pktstruct->conn_id;
    copy_utf16_to_bytes(cam_name, pktstruct->name);
    dbgser_states->printf("PTP recv'ed CMD-ACK, conn-ID 0x%08X, name: %s\r\n", conn_id, cam_name);
}

void PtpIpCamera::wait_while_busy(uint32_t min_time, uint32_t max_time, volatile bool* exit_signal)
{
    volatile bool to_exit = false;
    uint32_t start_time = millis();
    uint32_t now = start_time;
    while ((canSend() == false && (now - start_time) < max_time) || (now - start_time) < min_time) {
        now = millis();
        poll(); // poll, not task, because poll only reads and never sends
        // note: poll calls yield
        if (exit_signal != NULL)
        {
            to_exit |= *exit_signal;
            if (to_exit && (now - start_time) >= min_time) {
                break;
            }
        }
    }
}

#ifdef USE_ASYNC_SOCK
void PtpIpCamera::wait_canSend(AsyncClient* sock, uint32_t max_time)
{
    uint32_t start_time = millis();
    uint32_t now = start_time;
    while ((sock->canSend() == false && (now - start_time) < max_time)) {
        now = millis();
    }
}
#endif

void PtpIpCamera::reset_buffers()
{
    pktbuff_idx = 0;
    eventbuff_idx = 0;
    databuff_idx = 0;
}

bool PtpIpCamera::isKindaBusy()
{
    if (canNewConnect()) {
        return false;
    }
    if (isOperating()) {
        if (canSend() && pending_data <= 0) {
            return false;
        }
        else {
            return true;
        }
    }
    return true;
}

void PtpIpCamera::debug_rx(uint8_t* buff, uint32_t read_in) {
    uint32_t buff_addr = (uint32_t)buff;
    uint32_t buff_addr_pkt = (uint32_t)(this->pktbuff);
    uint32_t buff_addr_evt = (uint32_t)(this->eventbuff);
    uint32_t buff_len;
    if (buff_addr == buff_addr_pkt)
    {
        dbgser_rx->printf("RX[%u] PKT %u/%u:", millis(), read_in, (buff_len = this->pktbuff_idx));
    }
    else
    {
        dbgser_rx->printf("RX[%u] EVT %u/%u:", millis(), read_in, (buff_len = this->eventbuff_idx));
    }
    if (dbgser_rx->enabled) {
        print_buffer_hex(buff, buff_len);
    }
    dbgser_rx->printf("\r\n");
}

void PtpIpCamera::set_debugflags(uint32_t x)
{
    this->debug_flags = x;
    dbgser_states->        enabled = ((x & PTPDEBUGFLAG_STATES        ) != 0);
    dbgser_events->        enabled = ((x & PTPDEBUGFLAG_EVENTS        ) != 0);
    dbgser_rx->            enabled = ((x & PTPDEBUGFLAG_RX            ) != 0);
    dbgser_tx->            enabled = ((x & PTPDEBUGFLAG_TX            ) != 0);
    dbgser_devprop_dump->  enabled = ((x & PTPDEBUGFLAG_DEVPROP_DUMP  ) != 0);
    dbgser_devprop_change->enabled = ((x & PTPDEBUGFLAG_DEVPROP_CHANGE) != 0);
}

void PtpIpCamera::test_debug_msg(const char* s)
{
    dbgser_states->print(s);
}

#ifdef USE_ASYNC_SOCK

void PtpIpCamera::onAsyncPacket(void* pcam, AsyncClient* sock, struct pbuf* pb)
{
    PtpIpCamera* cam = (PtpIpCamera*)pcam;
    cam->poll_socket(sock, cam->pktbuff, &(cam->pktbuff_idx), PACKET_BUFFER_SIZE, pb);
    #ifdef PTPIP_KEEP_STATS
    cam->stats_pkts++;
    #endif
}

void PtpIpCamera::onAsyncPacketEvent(void* pcam, AsyncClient* sock, struct pbuf* pb)
{
    PtpIpCamera* cam = (PtpIpCamera*)pcam;
    cam->poll_socket(sock, cam->eventbuff, &(cam->eventbuff_idx), PACKET_BUFFER_SIZE, pb);
}

void PtpIpCamera::onAsyncError(void* pcam, AsyncClient* sock, int8_t errnum)
{
    PtpIpCamera* cam = (PtpIpCamera*)pcam;
    cam->error_cnt += 1;
    dbgser_important->printf("PTP socket error event 0x%02X\r\n", errnum);
}

void PtpIpCamera::onAsyncTimeout(void* pcam, AsyncClient* sock, uint32_t t)
{
    PtpIpCamera* cam = (PtpIpCamera*)pcam;
    cam->error_cnt += 1;
    dbgser_important->printf("PTP socket timeout event\r\n");
}

void PtpIpCamera::onAsyncAck(void* pcam, AsyncClient* sock, size_t sz, uint32_t t)
{
    PtpIpCamera* cam = (PtpIpCamera*)pcam;
    cam->error_cnt = 0;
    #ifdef PTPIP_KEEP_STATS
    cam->stats_acks++;
    #endif
}

#endif
