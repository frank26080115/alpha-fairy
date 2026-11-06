#ifndef _PTPIPCAMERATIME_H_
#define _PTPIPCAMERATIME_H_

#include <stddef.h>
#include <stdint.h>

// Local calendar time, with no implicit time-zone conversion.
struct ptpip_datetime_t
{
    uint16_t year;
    uint8_t month, day, hour, minute, second, decisecond;
};

bool ptpip_datetime_valid(const ptpip_datetime_t& value);
// Accept YYYY-MM-DD HH:MM:SS (or T separator), optionally followed by .s.
bool ptpip_datetime_parse(const char* text, ptpip_datetime_t* value);
bool ptpip_datetime_format(const ptpip_datetime_t& value, char* text, size_t size, bool fraction = false);
int64_t ptpip_datetime_seconds(const ptpip_datetime_t& value);
bool ptpip_sony_clock_decode(const uint8_t* data, size_t size, ptpip_datetime_t* value);
bool ptpip_sony_clock_encode(const ptpip_datetime_t& value, uint8_t payload[37]);

class PtpIpSonyAlphaCamera;

// Per-camera clock transactions. Kept separate from the camera's property/control API.
class PtpIpSonyClock
{
    public:
        typedef void (*time_callback_t)(bool success, const char* datetime);

    private:
        friend class PtpIpSonyAlphaCamera;
        bool get_time(PtpIpSonyAlphaCamera& camera, char* datetime, size_t size, uint32_t timeout_ms);
        bool set_time(PtpIpSonyAlphaCamera& camera, const char* datetime, uint32_t timeout_ms);
        bool request_time(PtpIpSonyAlphaCamera& camera, time_callback_t callback, uint32_t timeout_ms);
        bool request_set_time(PtpIpSonyAlphaCamera& camera, const char* datetime, time_callback_t callback, uint32_t timeout_ms);
        void task(PtpIpSonyAlphaCamera& camera);
        bool decode_pkt(PtpIpSonyAlphaCamera& camera, uint8_t* buff, uint32_t size);
        bool discard_pkt(const uint8_t* buff, uint32_t size) const;
        bool busy() const { return pending || callback != NULL; }
        void reset_session() { discard_late = false; }
        bool start_transaction(PtpIpSonyAlphaCamera& camera, uint32_t opcode, uint32_t* params,
                               uint8_t params_cnt, uint8_t* payload, int32_t payload_len);
        bool transaction(PtpIpSonyAlphaCamera& camera, uint32_t opcode, uint32_t* params,
                         uint8_t params_cnt, uint8_t* payload, int32_t payload_len, uint32_t timeout_ms);

        bool pending = false;
        bool reading = false;
        bool data_active = false;
        bool data_complete = false;
        bool data_valid = false;
        uint32_t transaction_id = 0;
        uint16_t response = 0;
        ptpip_datetime_t value = {};
        time_callback_t callback = NULL;
        uint32_t started_ms = 0, timeout_ms = 0;
        bool discard_late = false;
        uint32_t discard_id = 0;
};

#endif
