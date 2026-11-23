#include "PtpIpSonyAlphaCamera.h"
#include <Arduino.h>

bool PtpIpSonyClock::start_transaction(PtpIpSonyAlphaCamera& camera, uint32_t opcode, uint32_t* params,
    uint8_t params_cnt, uint8_t* payload, int32_t payload_len)
{
    if (busy() || !camera.canSend() || camera.state < PTPSTATE_POLLING || camera.state >= PTPSTATE_DISCONNECT
        || camera.properties_pending || camera.objinfo_pending) return false;

    reading = opcode == SONYALPHA_OPCODE_GetDateTime;
    data_active = false;
    data_complete = false;
    data_valid = false;
    response = 0;
    transaction_id = camera.transaction_id;
    pending = true;
    if (!camera.send_oper_req(opcode, params, params_cnt, payload, payload_len)) {
        pending = false;
        camera.force_disconnect(); // A partial write must not leave the next transaction out of sync.
        return false;
    }
    return true;
}

bool PtpIpSonyClock::transaction(PtpIpSonyAlphaCamera& camera, uint32_t opcode, uint32_t* params,
    uint8_t params_cnt, uint8_t* payload, int32_t payload_len, uint32_t wait_ms)
{
    if (callback) return false;
    camera.wait_while_busy(0, wait_ms);
    if (!start_transaction(camera, opcode, params, params_cnt, payload, payload_len)) return false;
    uint32_t started = millis();
    while (pending && camera.state >= PTPSTATE_POLLING && camera.state < PTPSTATE_DISCONNECT
        && (uint32_t)(millis() - started) < wait_ms) {
        // poll only: task/app_poll could send another operation or re-enter the CLI.
        camera.poll();
        yield();
    }
    if (pending) {
        pending = false;
        // Late responses/data cannot be safely reused by the next command.
        camera.force_disconnect();
        return false;
    }
    return response == PTP_RESPCODE_OK && (!reading || data_valid);
}

bool PtpIpSonyClock::request_time(PtpIpSonyAlphaCamera& camera, time_callback_t cb, uint32_t wait_ms)
{
    if (!cb || !start_transaction(camera, SONYALPHA_OPCODE_GetDateTime, NULL, 0, NULL, -1)) return false;
    callback = cb;
    started_ms = millis();
    timeout_ms = wait_ms;
    return true;
}

bool PtpIpSonyClock::request_set_time(PtpIpSonyAlphaCamera& camera, const char* datetime, int32_t utc_offset_minutes,
    time_callback_t cb, uint32_t wait_ms)
{
    ptpip_datetime_t date;
    uint8_t payload[47];
    if (!cb || !ptpip_datetime_parse(datetime, &date) || !ptpip_sony_clock_encode(date, utc_offset_minutes, payload)) return false;
    uint32_t property = SONYALPHA_PROPCODE_DateTimeSet;
    if (!start_transaction(camera, SONYALPHA_OPCODE_SetControlDeviceA, &property, 1, payload, sizeof(payload))) return false;
    callback = cb;
    started_ms = millis();
    timeout_ms = wait_ms;
    return true;
}

void PtpIpSonyClock::task(PtpIpSonyAlphaCamera& camera)
{
    if (!callback) return;
    if (pending && (camera.state < PTPSTATE_POLLING || camera.state >= PTPSTATE_DISCONNECT
        || (uint32_t)(millis() - started_ms) >= timeout_ms)) {
        // Abandon only this optional operation, keeping the camera connection usable.
        // Filter its late packets before they can touch a subsequent operation's data.
        discard_id = transaction_id;
        discard_late = true;
        pending = false;
        response = 0;
        camera.pending_data = 0;
        camera.databuff_idx = 0;
        if (camera.state >= PTPSTATE_POLLING && camera.state < PTPSTATE_DISCONNECT) camera.state &= ~1;
    }
    if (pending) return;
    bool success = response == PTP_RESPCODE_OK && (!reading || data_valid);
    char text[22];
    if (success && reading) success = ptpip_datetime_format(value, text, sizeof(text), true);
    time_callback_t cb = callback;
    callback = NULL;
    cb(success, success && reading ? text : NULL);
}

bool PtpIpSonyClock::decode_pkt(PtpIpSonyAlphaCamera& camera, uint8_t* buff, uint32_t size)
{
    if (!pending) return false;
    const ptpip_pkthdr_t* hdr = (const ptpip_pkthdr_t*)buff;
    uint32_t pkt_type = hdr->pkt_type;
    uint32_t pkt_len = hdr->length;
    if (pkt_type == PTP_PKTTYPE_STARTDATA && pkt_len >= sizeof(ptpip_pkt_startdata_t)) {
        uint32_t id, data_size, high_size;
        memcpy(&id, buff + 8, sizeof(id));
        memcpy(&data_size, buff + 12, sizeof(data_size));
        memcpy(&high_size, buff + 16, sizeof(high_size));
        data_active = id == transaction_id && data_size == 29 && high_size == 0;
        data_complete = false;
    }
    else if ((pkt_type == PTP_PKTTYPE_DATA || pkt_type == PTP_PKTTYPE_ENDDATA) && pkt_len >= 12) {
        uint32_t id;
        memcpy(&id, buff + 8, sizeof(id));
        if (id != transaction_id) data_active = false;
        if (pkt_type == PTP_PKTTYPE_ENDDATA) data_complete = data_active;
    }
    else if (pkt_type == PTP_PKTTYPE_OPERRESP && pkt_len >= 14) {
        uint32_t id;
        memcpy(&id, buff + 10, sizeof(id));
        if (id == transaction_id) {
            memcpy(&response, buff + 8, sizeof(response));
            if (reading && response == PTP_RESPCODE_OK && data_active && data_complete && camera.pending_data == 0) {
                data_valid = ptpip_sony_clock_decode_info(camera.databuff, camera.databuff_idx, &info);
                if (data_valid) value = info.datetime;
            }
            pending = false;
            camera.PtpIpCamera::decode_pkt(buff, size);
            return true;
        }
    }
    // Keep unrelated responses from completing a clock request or decoding its data as properties.
    if (pkt_type == PTP_PKTTYPE_OPERRESP) return true;
    if (pkt_type == PTP_PKTTYPE_ENDDATA) {
        camera.PtpIpCamera::decode_pkt(buff, size);
        return true;
    }
    return false;
}

bool PtpIpSonyClock::discard_pkt(const uint8_t* buff, uint32_t size) const
{
    if (!discard_late) return false;
    uint32_t type, id;
    memcpy(&type, buff + 4, sizeof(type));
    if (type == PTP_PKTTYPE_OPERRESP && size >= 14) memcpy(&id, buff + 10, sizeof(id));
    else if ((type == PTP_PKTTYPE_STARTDATA || type == PTP_PKTTYPE_DATA || type == PTP_PKTTYPE_ENDDATA
        || type == PTP_PKTTYPE_CANCELDATA) && size >= 12) memcpy(&id, buff + 8, sizeof(id));
    else return false;
    return id == discard_id;
}

bool PtpIpSonyClock::get_time(PtpIpSonyAlphaCamera& camera, char* datetime, size_t size, uint32_t wait_ms)
{
    if (!datetime || size < 22) return false;
    datetime[0] = '\0';
    if (!transaction(camera, SONYALPHA_OPCODE_GetDateTime, NULL, 0, NULL, -1, wait_ms)) return false;
    return ptpip_datetime_format(value, datetime, size, true);
}

bool PtpIpSonyClock::set_time(PtpIpSonyAlphaCamera& camera, const char* datetime, int32_t utc_offset_minutes, uint32_t wait_ms)
{
    ptpip_datetime_t date;
    uint8_t payload[47];
    if (!ptpip_datetime_parse(datetime, &date) || !ptpip_sony_clock_encode(date, utc_offset_minutes, payload)) return false;
    uint32_t property = SONYALPHA_PROPCODE_DateTimeSet;
    return transaction(camera, SONYALPHA_OPCODE_SetControlDeviceA, &property, 1, payload, sizeof(payload), wait_ms);
}

bool PtpIpSonyAlphaCamera::get_time(char* datetime, size_t size, uint32_t timeout_ms)
{
    return camera_time.get_time(*this, datetime, size, timeout_ms);
}

bool PtpIpSonyClock::get_info(PtpIpSonyAlphaCamera& camera, ptpip_sony_clock_info_t* out, uint32_t timeout_ms)
{
    if (!out || !transaction(camera, SONYALPHA_OPCODE_GetDateTime, NULL, 0, NULL, -1, timeout_ms)) return false;
    *out = info;
    return true;
}

bool PtpIpSonyClock::write_test(PtpIpSonyAlphaCamera& camera, const char* text, uint32_t timeout_ms)
{
    uint8_t payload[47];
    size_t length;
    if (!ptpip_sony_clock_encode_test(text, payload, sizeof(payload), &length)) return false;
    uint32_t property = SONYALPHA_PROPCODE_DateTimeSet;
    return transaction(camera, SONYALPHA_OPCODE_SetControlDeviceA, &property, 1, payload, length, timeout_ms);
}

bool PtpIpSonyAlphaCamera::get_time_info(ptpip_sony_clock_info_t* info, uint32_t timeout_ms)
{
    return camera_time.get_info(*this, info, timeout_ms);
}

bool PtpIpSonyAlphaCamera::test_time_write(const char* compact, uint32_t timeout_ms)
{
    return camera_time.write_test(*this, compact, timeout_ms);
}

bool PtpIpSonyAlphaCamera::set_time(const char* datetime, int32_t utc_offset_minutes, uint32_t timeout_ms)
{
    return camera_time.set_time(*this, datetime, utc_offset_minutes, timeout_ms);
}

bool PtpIpSonyAlphaCamera::request_time(time_callback_t callback, uint32_t timeout_ms)
{
    return camera_time.request_time(*this, callback, timeout_ms);
}

bool PtpIpSonyAlphaCamera::request_set_time(const char* datetime, int32_t utc_offset_minutes, time_callback_t callback, uint32_t timeout_ms)
{
    return camera_time.request_set_time(*this, datetime, utc_offset_minutes, callback, timeout_ms);
}

bool PtpIpSonyAlphaCamera::discard_pkt(const uint8_t* buff, uint32_t size)
{
    return camera_time.discard_pkt(buff, size);
}
