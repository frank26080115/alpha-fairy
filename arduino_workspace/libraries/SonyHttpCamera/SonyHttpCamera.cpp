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
    state = SHCAMSTATE_CONNECTING;
    dd_tries = 0;
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

    zoom_state = 0;
    zoom_time = 0;
    poll_delay = 10000;

    if (tbl_iso != NULL) {
        free(tbl_iso);
    }
    if (tbl_shutterspd != NULL) {
        free(tbl_shutterspd);
    }

    if (ip != 0) {
        get_dd_xml();
    }
}

void SonyHttpCamera::request_prep(void)
{
    //if (httpreq != NULL) {
    //    delete httpreq;
    //    httpreq = NULL;
    //}
    if (httpreq == NULL) {
        httpreq = new AsyncHTTPRequest();
    }
    rx_buff_idx = 0;
}

void SonyHttpCamera::request_close(void)
{
    //if (httpreq != NULL) {
    //    delete httpreq;
    //    httpreq = NULL;
    //}
}

void SonyHttpCamera::parse_dd_xml(char* data, int32_t maxlen)
{
    int32_t i, j, k;
    int32_t slen = maxlen;
    if (maxlen <= 0) {
        slen = strlen(data);
    }

    dbgser_rx->printf("dd.xml data %u:\r\n", slen);
    dbgser_rx->print(data);
    dbgser_rx->print("\r\n");

    char c;
    bool hasName = false;
    char keyFriendlyName[] = "<friendlyName>";
    int32_t keyLen = strlen(keyFriendlyName);
    for (i = 0; i < slen - keyLen; i++)
    {
        if (memcmp(&(data[i]), keyFriendlyName, keyLen) == 0)
        {
            dbgser_states->printf("found friendly name idx %u\r\n", i);
            i += keyLen;
            for (k = 0; i < slen - keyLen; i++, k++)
            {
                c = data[i];
                if (c == '<' && data[i + 1] == '/')
                {
                    friendly_name[k] = 0;
                    hasName = true;
                    break;
                }
                else
                {
                    friendly_name[k] = c;
                }
            }
            i += keyLen;
            break;
        }
    }

    if (hasName == false) {
        i = 0;
    }

    bool hasUrl = false;
    char keySrvTypeCam[] = "X_ScalarWebAPI_ServiceType>camera</";
    keyLen = strlen(keySrvTypeCam);
    for (; i < slen - keyLen && hasUrl == false; i++)
    {
        if (memcmp(&(data[i]), keySrvTypeCam, keyLen) == 0)
        {
            dbgser_states->printf("found ServiceType idx %u\r\n", i);
            i += keyLen;
            char keyActionListUrl[] = "ActionList_URL>http://";
            keyLen = strlen(keyActionListUrl);
            for (; i < slen - keyLen && hasUrl == false; i++)
            {
                if (memcmp(&(data[i]), keyActionListUrl, keyLen) == 0)
                {
                    printf("found ActionList_URL idx %u\r\n", i);
                    i += keyLen - 7;
                    for (k = 0; i < slen - keyLen; i++, k++)
                    {
                        c = data[i];
                        if (c == '<' && data[i + 1] == '/')
                        {
                            sprintf((char*)(&service_url[k]),"/camera");
                            hasUrl = true;
                            break;
                        }
                        else
                        {
                            service_url[k] = c;
                        }
                    }
                }
            }
        }
    }

    if (hasUrl == false) {
        i = 0;
    }

    char keyLiveView[] = "_LiveView_Single_URL>http://";
    keyLen = strlen(keyLiveView);
    for (; i < slen - keyLen; i++)
    {
        if (memcmp(&(data[i]), keyLiveView, keyLen) == 0)
        {
            if (liveview_url == NULL) {
                liveview_url = (char*)malloc(256);
            }
            dbgser_states->printf("found LiveView idx %u\r\n", i);
            i += keyLen - 7;
            for (k = 0; i < slen - keyLen; i++, k++)
            {
                c = data[i];
                if (c == '<' && data[i + 1] == '/')
                {
                    liveview_url[k] = 0;
                    break;
                }
                else
                {
                    liveview_url[k] = c;
                }
            }
        }
    }

    if (hasUrl)
    {
        dbgser_states->printf("service URL: %s\r\n", service_url);
        state = SHCAMSTATE_INIT_GOTDD;
    }
    else
    {
        dbgser_states->printf("no service URL found\r\n");
        state = SHCAMSTATE_FORBIDDEN;
    }
}

void SonyHttpCamera::get_dd_xml()
{
    sprintf(url_buffer, "http://%s:64321/dd.xml", IPAddress(ip_addr).toString().c_str());
    dbgser_states->printf("getting dd.xml from URL: %s\r\n", url_buffer);
    request_prep();
    httpreq->onData(NULL, NULL);
    httpreq->onReadyStateChange(ddRequestCb, this);
    bool openres = httpreq->open("GET", url_buffer);
    if (openres) {
        httpreq->send();
        state = SHCAMSTATE_CONNECTING;
    }
    else {
        dbgser_states->printf("httpcam unable to get dd.xml\r\n");
    }
}

void SonyHttpCamera::ddRequestCb(void* optParm, AsyncHTTPRequest* req, int readyState)
{
    SonyHttpCamera* cam = (SonyHttpCamera*)optParm;
    if (cam->state == SHCAMSTATE_FORBIDDEN) {
        return;
    }
    if (readyState == readyStateDone) 
    {
        int respcode;
        if ((respcode = req->responseHTTPcode()) == 200)
        {
            dbgser_states->printf("got dd.xml\r\n");
            // expect about 3 kilobytes of data
            cam->parse_dd_xml(req->responseLongText());
        }
        else
        {
            dbgser_states->printf("failed to get dd.xml, code %d", respcode);
            cam->dd_tries++;
            if (cam->dd_tries < 3) {
                dbgser_states->printf(", retrying (%u)\r\n", cam->dd_tries);
                cam->get_dd_xml();
            }
            else {
                dbgser_states->printf(", giving up (%u)\r\n", cam->dd_tries);
                cam->state = SHCAMSTATE_FAILED;
            }
        }
        cam->request_close();
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
    rx_buff_idx = 0;
    request_prep();
    httpreq->onData(eventDataCb, this);
    httpreq->onReadyStateChange(eventRequestCb, this);
    bool openres = httpreq->open("POST", url_buffer);
    if (openres)
    {
        //httpreq->setReqHeader("Content-Type", "application/json"); // this causes error 415
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

void SonyHttpCamera::task()
{
    poll();
}

void SonyHttpCamera::poll()
{
    uint32_t now;

    if (state == SHCAMSTATE_FORBIDDEN) {
        return;
    }

    if (state == SHCAMSTATE_INIT_GOTDD)
    {
        state = SHCAMSTATE_INIT_STARTRECMODE;
        dd_tries = 0;
    }

    if ((state == SHCAMSTATE_INIT_STARTRECMODE || state == SHCAMSTATE_INIT_SETCAMFUNC) && (httpreq->readyState() == readyStateUnsent || httpreq->readyState() == readyStateDone))
    {
        request_prep();
        httpreq->onData(NULL, NULL);
        httpreq->onReadyStateChange(initRequestCb, this);
        bool openres = httpreq->open("POST", url_buffer);
        if (openres)
        {
            //httpreq->setReqHeader("Content-Type", "application/json"); // this causes error 415
            if (state == SHCAMSTATE_INIT_STARTRECMODE) {
                sprintf(cmd_buffer, "{\r\n    \"method\": \"startRecMode\",\r\n    \"params\": [],\r\n    \"id\": %u,\r\n    \"version\": \"1.0\"\r\n}", req_id);
                httpreq->send(cmd_buffer);
                req_id++;
                dbgser_tx->printf("httpcam init startRecMode\r\n");
            }
            else if (state == SHCAMSTATE_INIT_SETCAMFUNC) {
                sprintf(cmd_buffer, "{\r\n    \"method\": \"cameraFunction\",\r\n    \"params\": [\"Remote Shooting\"],\r\n    \"id\": %u,\r\n    \"version\": \"1.0\"\r\n}", req_id);
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
        now = millis();
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

void SonyHttpCamera::wait_while_busy(uint32_t min_time, uint32_t max_time, volatile bool* exit_signal)
{
    volatile bool to_exit = false;
    uint32_t start_time = millis();
    uint32_t now = start_time;
    uint32_t old_poll_delay = poll_delay;
    poll_delay = 50;
    while ((canSend() == false && isOperating() == true && (now - start_time) < max_time) || (now - start_time) < min_time) {
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
    poll_delay = old_poll_delay;
}

void SonyHttpCamera::initRequestCb(void* optParm, AsyncHTTPRequest* req, int readyState)
{
    SonyHttpCamera* cam = (SonyHttpCamera*)optParm;

    if (cam->state == SHCAMSTATE_FORBIDDEN) {
        return;
    }

    if (readyState == readyStateDone) 
    {
        int rcode;
        int do_next = 0;
        if ((rcode = req->responseHTTPcode()) == 200)
        {
            dbgser_states->printf("httpcam init state %u done\r\n", cam->state);
            do_next = cam->state + 1; // go to next step
            cam->dd_tries = 0;
        }
        else
        {
            dbgser_states->printf("httpcam init state %u error %d", cam->state, rcode);
            cam->dd_tries++;
            if (cam->dd_tries < 2) {
                dbgser_states->printf(", retry %d\r\n", cam->dd_tries);
                do_next = cam->state - 1; // retry previous step
            }
            else {
                dbgser_states->printf(", skipping %d, do next\r\n", cam->dd_tries);
                //cam->state = SHCAMSTATE_FAILED;
                do_next = cam->state + 1;
                cam->dd_tries = 0;
            }
        }
        if (do_next != 0)
        {
            cam->state = do_next; // next call to poll will handle it
        }
        cam->request_close();
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
            cam->state &= 0xFE;
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
