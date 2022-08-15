#include "SonyHttpCamera.h"

DebuggingSerial* SonyHttpCamera::dbgser_important;
DebuggingSerial* SonyHttpCamera::dbgser_states;
DebuggingSerial* SonyHttpCamera::dbgser_events;
DebuggingSerial* SonyHttpCamera::dbgser_rx;
DebuggingSerial* SonyHttpCamera::dbgser_tx;
DebuggingSerial* SonyHttpCamera::dbgser_devprop_dump;
DebuggingSerial* SonyHttpCamera::dbgser_devprop_change;

SonyHttpCamera::SonyHttpCamera()
{
    tbl_iso = NULL;
    tbl_shutterspd = NULL;

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
    dbgser_rx->       enabled = true;
    dbgser_tx->       enabled = true;
    dbgser_devprop_dump->  enabled = true;
    dbgser_devprop_change->enabled = true;

    begin(0);
}

void SonyHttpCamera::begin(uint32_t ip)
{
    ip_addr = ip;
    init_retries = 0;
    error_cnt = 0;
    event_api_version = 0;
    is_movierecording_v = false;
    is_manuallyfocused_v = false;
    is_focused = false;
    req_id = 1;
    rx_buff_idx = 0;
    rx_buff[0] = 0;
    last_poll_time = 0;
    if (state != SHCAMSTATE_FORBIDDEN || ip == 0) {
        state = SHCAMSTATE_NONE;
    }
    else {
        state = SHCAMSTATE_WAIT;
        state_after_wait = SHCAMSTATE_CONNECTING;
        wait_until = millis() + 500;
    }

    zoom_state = 0;
    zoom_time = 0;
    poll_delay = 10000;

    if (tbl_iso != NULL) {
        free(tbl_iso);
    }
    if (tbl_shutterspd != NULL) {
        free(tbl_shutterspd);
    }
}

bool SonyHttpCamera::parse_event(char* data, int32_t maxlen)
{
    char res_buff[64] = {0};
    signed int i, j, k;

    bool ret = false;

    bool found;

    found = scan_json_for_key(rx_buff, rx_buff_idx, "cameraStatus", &i, &j, (char*)res_buff, 64);
    if (found) {
        is_movierecording_v = (memcmp(res_buff, "Movie", 5) == 0);
        ret |= true;
        event_found_flag |= (1 << 0);
    }

    found = scan_json_for_key(rx_buff, rx_buff_idx, "focusStatus", &i, &j, (char*)res_buff, 64);
    if (found) {
        strcpy(str_focusstatus, res_buff);
        is_focused = (memcmp(res_buff, "Focused", 7) == 0);
        ret |= true;
        event_found_flag |= (1 << 1);
    }

    found = scan_json_for_key(rx_buff, rx_buff_idx, "currentIsoSpeedRate", &i, &j, (char*)res_buff, 64);
    if (found && strlen(res_buff) > 0) {
        strcpy(str_iso, res_buff);
        ret |= true;
        event_found_flag |= (1 << 2);
    }

    if (tbl_iso == NULL) {
        found = scan_json_for_key(rx_buff, rx_buff_idx, "isoSpeedRateCandidates", &i, &j, NULL, 0);
        if (found) {
            k = j - i + 3;
            tbl_iso = (char*)malloc(k);
            found = scan_json_for_key(rx_buff, rx_buff_idx, "isoSpeedRateCandidates", &i, &j, (char*)tbl_iso, k - 1);
            ret |= true;
            event_found_flag |= (1 << 3);
        }
    }

    found = scan_json_for_key(rx_buff, rx_buff_idx, "currentShutterSpeed", &i, &j, (char*)res_buff, 64);
    if (found && strlen(res_buff) > 0) {
        strcpy(str_shutterspd, res_buff);
        strcpy_no_slash(str_shutterspd_clean, str_shutterspd);
        ret |= true;
        event_found_flag |= (1 << 4);
    }

    if (tbl_shutterspd == NULL) {
        found = scan_json_for_key(rx_buff, rx_buff_idx, "shutterSpeedCandidates", &i, &j, NULL, 0);
        if (found) {
            k = j - i + 3;
            tbl_shutterspd = (char*)malloc(k);
            found = scan_json_for_key(rx_buff, rx_buff_idx, "shutterSpeedCandidates", &i, &j, (char*)tbl_shutterspd, k - 1);
            ret |= true;
            event_found_flag |= (1 << 5);
        }
    }

    found = scan_json_for_key(rx_buff, rx_buff_idx, "currentFocusMode", &i, &j, (char*)res_buff, 64);
    if (found && strlen(res_buff) > 0) {
        is_manuallyfocused_v = (memcmp(res_buff, "MF", 2) == 0);
        if (is_manuallyfocused_v == false) {
            strcpy(str_afmode, res_buff);
        }
        ret |= true;
        event_found_flag |= (1 << 6);
    }

    found = scan_json_for_key(rx_buff, rx_buff_idx, "currentShootMode", &i, &j, (char*)res_buff, 64);
    if (found && strlen(res_buff) > 0) {
        if (memcmp("still", res_buff, 5) == 0) {
            shoot_mode == SHOOTMODE_STILLS;
        }
        else if (memcmp("movie", res_buff, 5) == 0) {
            shoot_mode == SHOOTMODE_MOVIE;
        }
    }
    return ret;
}

void SonyHttpCamera::eventRequestCb(void* optParm, AsyncHTTPRequest* req, int readyState)
{
    SonyHttpCamera* cam = (SonyHttpCamera*)optParm;

    if (cam->state == SHCAMSTATE_FORBIDDEN) {
        return;
    }

    dbgser_rx->printf("httpcam rx eventRequestCb %d %u\r\n", readyState, req->available());

    int32_t chunk;
    while ((chunk = req->available()) > 0)
    {
        int32_t r = read_in_chunk(req, chunk, cam->rx_buff, &(cam->rx_buff_idx));
        if (r == 0) {
            break;
        }
        cam->parse_event(cam->rx_buff, cam->rx_buff_idx);
    }

    if (readyState == readyStateDone)
    {
        int respcode;
        if ((respcode = req->responseHTTPcode()) == 200)
        {
            dbgser_devprop_dump->printf("\r\nhttpcam polled event 0x%08X\r\n", cam->event_found_flag);
            if (strlen(cam->str_focusstatus) <= 0) {
                if (cam->event_api_version > 1) {
                    cam->event_api_version--;
                }
            }
            cam->error_cnt = 0;
            if (cam->state < SHCAMSTATE_READY) {
                dbgser_states->printf("httpcam connected\r\n");
                if (cam->cb_onConnect != NULL) {
                    cam->cb_onConnect();
                }
            }
            cam->state = SHCAMSTATE_READY;
        }
        else
        {
            dbgser_states->printf("httpcam event resp err %d\r\n", respcode);
            cam->error_cnt++;
        }
        cam->request_close();
    }
}

void SonyHttpCamera::eventDataCb(void* optParm, AsyncHTTPRequest* req, int avail)
{
    SonyHttpCamera* cam = (SonyHttpCamera*)optParm;

    if (cam->state == SHCAMSTATE_FORBIDDEN) {
        return;
    }
    if (req->available() <= 0) {
        dbgser_rx->printf("httpcam rx eventDataCb %u\r\n", req->available());
        return;
    }
    dbgser_rx->printf("httpcam rx eventDataCb %u\r\n", req->available());

    int32_t chunk;
    while ((chunk = req->available()) > 0)
    {
        int32_t r = read_in_chunk(req, chunk, cam->rx_buff, &(cam->rx_buff_idx));
        if (r == 0) {
            break;
        }
        cam->parse_event(cam->rx_buff, cam->rx_buff_idx);
    }
}

void SonyHttpCamera::get_event()
{
    event_found_flag = 0;
    bool openres = request_prep("POST", url_buffer, "application/json", eventRequestCb, eventDataCb);
    if (openres)
    {
        sprintf(cmd_buffer, "{\r\n    \"method\": \"getEvent\",\r\n    \"params\": [false],\r\n    \"id\": %u,\r\n    \"version\": \"1.%u\"\r\n}", req_id, event_api_version);
        dbgser_tx->printf("httpcam get_event json: %s\r\n", cmd_buffer);
        httpreq->send(cmd_buffer);
        req_id++;
        state = SHCAMSTATE_POLLING;
        last_poll_time = millis();
    }
    else
    {
        dbgser_tx->printf("httpcam unable to send get_event\r\n");
    }
}

void SonyHttpCamera::poll()
{
    yield();
}

void SonyHttpCamera::task()
{
    if (state == SHCAMSTATE_FORBIDDEN) {
        return;
    }

    uint32_t now = millis();

    poll();

    if (state == SHCAMSTATE_WAIT && now >= wait_until) {
        state = state_after_wait;
        if (state == SHCAMSTATE_CONNECTING && ip_addr != 0) {
            ssdp_start();
            wait_until = now + 1000;
            init_retries = 0;
        }
    }

    if (state == SHCAMSTATE_INIT_SSDP)
    {
        bool got_ssdp = false;
        if (ssdp_udp.available() > 0)
        {
            got_ssdp = true;
        }
        else if (now >= wait_until)
        {
            ssdp_udp.parsePacket();
            if (ssdp_udp.available() > 0) {
                got_ssdp = true;
            }
            if (got_ssdp == false) {
                init_retries++;
                if (init_retries >= 2) {
                    got_ssdp = true; // trick into getting dd.xml
                }
                else {
                    ssdp_start();
                    wait_until = now + 1000;
                }
            }
        }
        if (got_ssdp) {
            init_retries = 0;
            get_dd_xml();
        }
    }

    if (state == SHCAMSTATE_INIT_GOTDD)
    {
        state = SHCAMSTATE_INIT_GETVERSION;
        init_retries = 0;
    }

    if ((state == SHCAMSTATE_INIT_GETVERSION || state == SHCAMSTATE_INIT_GETAPILIST || state == SHCAMSTATE_INIT_STARTRECMODE || state == SHCAMSTATE_INIT_SETCAMFUNC)
        && (httpreq->readyState() == readyStateUnsent || httpreq->readyState() == readyStateDone))
    {
        bool openres = request_prep("POST", url_buffer, "application/json", initRequestCb, NULL);
        if (openres)
        {
            if (state == SHCAMSTATE_INIT_GETVERSION) {
                sprintf(cmd_buffer, cmd_generic_fmt, "getVersions", req_id);
                httpreq->send(cmd_buffer);
                req_id++;
                dbgser_tx->printf("httpcam init getVersions\r\n");
            }
            else if (state == SHCAMSTATE_INIT_GETAPILIST) {
                sprintf(cmd_buffer, cmd_generic_fmt, "getAvailableApiList", req_id);
                httpreq->send(cmd_buffer);
                req_id++;
                dbgser_tx->printf("httpcam init getAvailableApiList\r\n");
            }
            else if (state == SHCAMSTATE_INIT_STARTRECMODE) {
                sprintf(cmd_buffer, cmd_generic_fmt, "startRecMode", req_id);
                httpreq->send(cmd_buffer);
                req_id++;
                dbgser_tx->printf("httpcam init startRecMode\r\n");
            }
            else if (state == SHCAMSTATE_INIT_SETCAMFUNC) {
                sprintf(cmd_buffer, cmd_generic_fmt, "cameraFunction", req_id);
                httpreq->send(cmd_buffer);
                req_id++;
                dbgser_tx->printf("httpcam init cameraFunction\r\n");
            }
            state++; // set the flag for waiting
            last_poll_time = millis();
        }
    }

    if (state == SHCAMSTATE_INIT_DONE)
    {
        get_event();
    }

    if (state == SHCAMSTATE_READY || state == SHCAMSTATE_POLLING)
    {
        if ((now - last_poll_time) > poll_delay)
        {
            get_event();
        }
        else if ((now - zoom_time) > 5000)
        {
            cmd_ZoomStop();
        }
    }

    if (error_cnt > 10) {
        state = SHCAMSTATE_FAILED;
        if (cb_onDisconnect != NULL) {
            cb_onDisconnect();
        }
    }
}

void SonyHttpCamera::genericRequestCb(void* optParm, AsyncHTTPRequest* req, int readyState)
{
    SonyHttpCamera* cam = (SonyHttpCamera*)optParm;

    if (cam->state == SHCAMSTATE_FORBIDDEN) {
        return;
    }

    if (readyState == readyStateDone) 
    {
        int rcode;
        if ((rcode = req->responseHTTPcode()) == 200)
        {
            cam->state &= 0xFE; // only clear the wait flag
            cam->error_cnt = 0;
        }
        else
        {
            cam->error_cnt++;
        }
        cam->request_close();
    }
}

void SonyHttpCamera::set_debugflags(uint32_t x)
{
    debug_flags = x;
    dbgser_states->        enabled = ((x & DEBUGFLAG_STATES        ) != 0);
    dbgser_events->        enabled = ((x & DEBUGFLAG_EVENTS        ) != 0);
    dbgser_rx->            enabled = ((x & DEBUGFLAG_RX            ) != 0);
    dbgser_tx->            enabled = ((x & DEBUGFLAG_TX            ) != 0);
    dbgser_devprop_dump->  enabled = ((x & DEBUGFLAG_DEVPROP_DUMP  ) != 0);
    dbgser_devprop_change->enabled = ((x & DEBUGFLAG_DEVPROP_CHANGE) != 0);
}

void SonyHttpCamera::test_debug_msg(const char* s)
{
    dbgser_states->print(s);
}
