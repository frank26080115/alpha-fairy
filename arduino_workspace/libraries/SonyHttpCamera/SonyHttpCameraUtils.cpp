#include "SonyHttpCamera.h"

void read_in_chunk(AsyncHTTPRequest* req, int32_t chunk, char* buff, int32_t* buff_idx)
{
    if (chunk > SHCAM_RXBUFF_UNIT) {
        chunk = SHCAM_RXBUFF_UNIT;
    }
    int32_t i = (*buff_idx) + chunk;
    while (i > SHCAM_RXBUFF_SIZE) {
        int32_t j, k;
        for (j = 0, k = SHCAM_RXBUFF_UNIT; k < (*buff_idx); j++, k++) {
            buff[j] = buff[k];
        }
        i -= SHCAM_RXBUFF_UNIT;
        (*buff_idx) -= SHCAM_RXBUFF_UNIT;
    }
    int32_t r = req->responseRead((uint8_t*)(&(buff[(*buff_idx)])), chunk);
    (*buff_idx) += r;
    buff[(*buff_idx)] = 0;
    return r;
}

bool scan_json_for_key(char* data, int32_t datalen, char* keystr, signed int* start_idx, signed int* end_idx, char* tgt, int tgtlen)
{
    int32_t slen = datalen;
    if (datalen <= 0) {
        slen = strlen(data);
    }
    signed int sidx = -1, eidx = -1;
    *start_idx = -1;
    *end_idx = -1;

    int32_t keylen = strlen(keystr);
    int i, j;
    bool found = false;
    for (i = 0; i < slen && found == false; i++)
    {
        if (data[i] == '"' && data[i + keylen + 2] == '"')
        {
            if (memcmp(&(data[i + 1]), keystr, keylen) == 0)
            {
                i += keylen + 2;
                char c;
                for (; i < slen && c != 0 && found == false; i++)
                {
                    c = data[i];
                    if (c == ':')
                    {
                        i++;
                        sidx = i;
                        for (; i < slen && c != 0 && found == false; i++)
                        {
                            c = data[i];
                            if (c == '"')
                            {
                                i++;
                                sidx = i;
                                i++;
                                char prev_c = c;
                                for (; i < slen && c != 0 && found == false; i++)
                                {
                                    c = data[i];
                                    if (c == '"' && prev_c != '\\')
                                    {
                                        eidx = i - 1;
                                        found = true;
                                    }
                                    prev_c = c;
                                }
                            }
                            else if (c == '[')
                            {
                                i++;
                                sidx = i;
                                i++;
                                bool in_quote = false;
                                char prev_c = c;
                                for (; i < slen && c != 0 && found == false; i++)
                                {
                                    c = data[i];
                                    if (c == '"' && prev_c != '\\')
                                    {
                                        in_quote ^= true;
                                    }
                                    else if (c == ']' && in_quote == false)
                                    {
                                        eidx = i - 1;
                                        found = true;
                                    }
                                    prev_c = c;
                                }
                            }
                            else if (c == ',')
                            {
                                eidx = i - 1;
                                found = true;
                            }
                            else if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                            {
                                if (i == sidx + 1) {
                                    i++;
                                    sidx = i;
                                }
                            }
                        }
                    }
                    else if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
                    {
                        return false;
                    }
                }
            }
        }
    }
    if (sidx < 0 || eidx < 0) {
        return false;
    }
    if (found) {
        if (start_idx != NULL) {
            *start_idx = sidx;
        }
        if (end_idx != NULL) {
            *end_idx = sidx;
        }
        if (tgt != NULL) {
            for (i = sidx, j = 0; i <= eidx && j < tgtlen - 1; i++, j++) {
                tgt[j] = data[i];
            }
            tgt[j] = 0;
        }
    }
    return found;
}

int count_commas(char* data)
{
    uint32_t slen = strlen(data);
    int i, cnt = 0;
    for (i = 0; i < slen; i++) {
        char c = data[i];
        if (c == ',') {
            cnt++;
        }
    }
    return cnt;
}

void parse_all_shutter_speeds(uint32_t* data_table, uint32_t table_len, char* json_data)
{
    int tbl_i, json_i, n_i, d_i;
    int json_len = strlen(json_data);
    bool is_denom = false;
    char numstr[32];
    char denomstr[32];
    for (tbl_i = 0, json_i = 0, n_i = 0, d_i = 0; tbl_i < table_len && json_i < json_len; json_i++)
    {
        char c = json_data[json_i];
        if (c == ',' || json_data[json_i + 1] == 0)
        {
            float numerator
            if (n_i == 0)
            {
                is_denom = false;
                n_i = 0, d_i = 0;
                continue;
            }
            else if (d_i == 0)
            {
            }

            is_denom = false;
            n_i = 0, d_i = 0;
        }
        else if (c == '/')
        {
            is_denom = true;
        }
        else if (c == '\\' || c == '"' || c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            continue;
        }
    }
}