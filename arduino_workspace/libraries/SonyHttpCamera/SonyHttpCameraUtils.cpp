#include "SonyHttpCamera.h"

int SonyHttpCamera::read_in_chunk(AsyncHTTPRequest* req, int32_t chunk, char* buff, uint32_t* buff_idx)
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
    uint8_t* tgt = (uint8_t*)(&(buff[(*buff_idx)]));
    if ((*buff_idx) == 0) {
        dbgser_rx->println("httpcam rx: ");
    }
    int32_t r = req->responseRead(tgt, chunk);
    dbgser_rx->write(tgt, r);
    (*buff_idx) += r;
    buff[(*buff_idx)] = 0;
    return r;
}

bool scan_json_for_key(char* data, int32_t datalen, const char* keystr, signed int* start_idx, signed int* end_idx, char* tgt, int tgtlen)
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
                    else if (c == ',')
                    {
                        break;
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
            *end_idx = eidx;
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

int get_idx_within_strtbl(char* tbl, char* needle)
{
    int slen = strlen(tbl);
    int nlen = strlen(needle);
    int i;
    int comma_cnt = 0;
    bool in_quote = false;
    char c, prev_c = 0;
    for (i = 0; i < slen - nlen; i++)
    {
        c = tbl[i];
        if (c == '"' && tbl[i + nlen + 2] == '"')
        {
            prev_c = c;
            i++;
            if (memcmp(&tbl[i], needle, nlen) == 0) {
                return comma_cnt;
            }
            continue;
        }
        else if (c == '"' && prev_c != '\\')
        {
            in_quote ^= true;
        }
        else if (c == ',' && in_quote == false)
        {
            comma_cnt++;
        }
        prev_c = c;
    }
    return -1;
}

bool get_txt_within_strtbl(char* tbl, int idx, char* tgt)
{
    int slen = strlen(tbl);
    int i, j;
    int comma_cnt = 0;
    bool in_quote = false;
    char c, prev_c = 0;
    for (i = 0, j = 0; i < slen; i++)
    {
        c = tbl[i];
        if (c == '"' && prev_c != '\\') {
            in_quote ^= true;
        }
        else if (c == ',' && in_quote == false)
        {
            comma_cnt++;
        }
        else if (in_quote && comma_cnt == idx)
        {
            tgt[j]     = c;
            tgt[j + 1] = 0;
        }

        if (in_quote == false && j > 0)
        {
            return true;
        }
        prev_c = c;
    }
    if (j > 0) {
        return true;
    }
    tgt[0] = 0;
    return false;
}

void strcpy_no_slash(char* dst, char* src)
{
    int i, j;
    int slen = strlen(src);
    for (i = 0, j = 0; i < slen; i++)
    {
        char c = src[i];
        if (c != '\\') {
            dst[j] = c;
            dst[j+1] = 0;
            j += 1;
        }
    }
}

uint32_t parse_shutter_speed_str(char* s)
{
    int slen = strlen(s);
    if (s[slen - 1] == '"')
    {
        s[slen - 1] = 0;
        slen--;
        if (s[slen - 1] == '\\') {
            s[slen - 1] = 0;
            slen--;
        }
        double f = atof(s);
        f *= 10;
        int fi = lround(f);
        fi <<= 16;
        fi |= 10;
        return fi;
    }
    else if (memcmp("1/", s, 2) == 0)
    {
        int x = atoi(&(s[2]));
        x |= 0x10000;
        return x;
    }
    return 0;
}

uint32_t SonyHttpCamera::get_another_shutterspd(int idx, char* tgt)
{
    if (get_txt_within_strtbl(str_shutterspd, idx, tgt)) {
        return parse_shutter_speed_str(tgt);
    }
    return 0;
}
