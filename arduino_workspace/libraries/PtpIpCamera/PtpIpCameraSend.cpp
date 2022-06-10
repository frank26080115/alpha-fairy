#include "PtpIpCamera.h"
#include <Arduino.h>
#include "ptpip_utils.h"

#if 0
#define PTPSEND_DEBUG(x) send_debug(x)
#else
#define PTPSEND_DEBUG(x)
#endif

bool PtpIpCamera::send_oper_req(uint32_t opcode, uint32_t* params, uint8_t params_cnt, uint8_t* payload, int32_t payload_len)
{
    ptpip_pkt_operreq_t* pktstruct = (ptpip_pkt_operreq_t*)outbuff;
    uint32_t* params_dest = (uint32_t*)&(outbuff[sizeof(ptpip_pkt_operreq_t)]);
    uint32_t len, i;
    int wrote;
    pktstruct->header.pkt_type = PTP_PKTTYPE_OPERREQ;
    pktstruct->data_phase = payload_len != 0 ? 1 : 0;
    pktstruct->op_code = opcode;
    pktstruct->transaction_id = transaction_id;
    memcpy(params_dest, params, params_cnt * sizeof(uint32_t));
    len = sizeof(ptpip_pkt_operreq_t) + (params_cnt * sizeof(uint32_t));
    pktstruct->header.length = len;
    PTPSEND_DEBUG("PTPSEND OPER_REQ");
    wrote = socket_main.write((const uint8_t*)outbuff, (size_t)len);
    if (wrote > 0) {
        if (payload_len > 0) {
            if (send_data(payload, payload_len) == false) {
                error_cnt += 1;
                return false;
            }
        }
        transaction_id += 1;
        state |= 1;
        error_cnt = 0;
        return true;
    }
    else {
        error_cnt += 1;
        return false;
    }
}

bool PtpIpCamera::send_data(uint8_t* payload, uint32_t payload_len)
{
    int wrote;

    ptpip_pkt_startdata_t* startstruct = (ptpip_pkt_startdata_t*)outbuff;
    startstruct->header.pkt_type = PTP_PKTTYPE_STARTDATA;
    startstruct->transaction_id = transaction_id;
    startstruct->pending_data_length = payload_len;
    startstruct->header.length = sizeof(ptpip_pkt_startdata_t);
    PTPSEND_DEBUG("PTPSEND START_DATA");
    wrote = socket_main.write((const uint8_t*)outbuff, (size_t)(startstruct->header.length));
    if (wrote <= 0) {
        error_cnt += 1;
        return false;
    }

    ptpip_pkt_data_t* datastruct = (ptpip_pkt_data_t*)outbuff;
    datastruct->header.pkt_type = PTP_PKTTYPE_DATA;
    datastruct->transaction_id = transaction_id;
    memcpy((void*)&(outbuff[sizeof(ptpip_pkt_data_t)]), payload, payload_len);
    datastruct->header.length = sizeof(ptpip_pkt_data_t) + payload_len;
    PTPSEND_DEBUG("PTPSEND DATA");
    wrote = socket_main.write((const uint8_t*)outbuff, (size_t)(startstruct->header.length));
    if (wrote <= 0) {
        error_cnt += 1;
        return false;
    }

    ptpip_pkt_enddata_t* endstruct = (ptpip_pkt_enddata_t*)outbuff;
    endstruct->header.pkt_type = PTP_PKTTYPE_ENDDATA;
    endstruct->transaction_id = transaction_id;
    endstruct->header.length = sizeof(ptpip_pkt_enddata_t);
    PTPSEND_DEBUG("PTPSEND END_DATA");
    wrote = socket_main.write((const uint8_t*)outbuff, (size_t)(endstruct->header.length));
    if (wrote <= 0) {
        error_cnt += 1;
        return false;
    }

    error_cnt = 0;
    return true;
}

bool PtpIpCamera::send_cmd_req()
{
    int wrote;
    uint32_t len;
    uint32_t* version_ptr;
    ptpip_pkt_cmdreq_t* pktstruct = (ptpip_pkt_cmdreq_t*)outbuff;
    pktstruct->header.pkt_type = PTP_PKTTYPE_INITCMDREQ;
    uint8_t guid_src[16];
    WiFi.macAddress(guid_src);
    memcpy(&(guid_src[6]), my_name, 10);
    memcpy(pktstruct->guid, guid_src, 16);
    len = 8 + 16;
    len += copy_bytes_to_utf16(my_name, pktstruct->name);
    version_ptr = (uint32_t*)&(outbuff[len]);
    (*version_ptr) = 0x00010000;
    len += 4;
    pktstruct->header.length = len;
    PTPSEND_DEBUG("PTPSEND CMD_REQ");
    wrote = socket_main.write((const uint8_t*)outbuff, (size_t)(len));
    if (wrote > 0) {
        state |= 1;
        error_cnt = 0;
        return true;
    }
    else {
        error_cnt += 1;
        return false;
    }
}

bool PtpIpCamera::send_event_req()
{
    int wrote;
    ptpip_pkt_eventreq_t* pktstruct = (ptpip_pkt_eventreq_t*)outbuff;
    pktstruct->header.pkt_type = PTP_PKTTYPE_INITEVENTREQ;
    pktstruct->conn_id = conn_id;
    pktstruct->header.length = sizeof(ptpip_pkt_eventreq_t);
    PTPSEND_DEBUG("PTPSEND EVENT_REQ");
    wrote = socket_event.write((const uint8_t*)outbuff, (size_t)(pktstruct->header.length));
    if (wrote > 0) {
        state |= 1;
        error_cnt = 0;
        return true;
    }
    else {
        error_cnt += 1;
        return false;
    }
}

bool PtpIpCamera::send_probe_resp()
{
    int wrote;
    ptpip_pkthdr_t* pktstruct = (ptpip_pkthdr_t*)outbuff;
    pktstruct->pkt_type = PTP_PKTTYPE_PROBERESP;
    pktstruct->length = sizeof(ptpip_pkthdr_t);
    PTPSEND_DEBUG("PTPSEND PROBE_RESP");
    wrote = socket_event.write((const uint8_t*)outbuff, (size_t)(pktstruct->length));
    if (wrote > 0) {
        error_cnt = 0;
        return true;
    }
    else {
        error_cnt += 1;
        return false;
    }
}

bool PtpIpCamera::send_open_session()
{
    uint32_t params[2];
    session_id += 1;
    transaction_id = 1;
    params[0] = transaction_id;
    params[1] = session_id;
    bool success = send_oper_req(PTP_OPCODE_OpenSession, params, 2, NULL, -1);
    if (success) {
        error_cnt = 0;
    }
    return success;
}

void PtpIpCamera::send_debug(char* s)
{
    Serial.print(s);
    Serial.print(" ");
    uint32_t* len = (uint32_t*)outbuff;
    print_buffer_hex(outbuff, *len);
    Serial.println();
}
