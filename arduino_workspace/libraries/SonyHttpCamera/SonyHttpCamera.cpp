#include "SonyHttpCamera.h"

SonyHttpCamera::SonyHttpCamera()
{
    iso_table = NULL;
    shutterspd_table = NULL;
    begin(0);
}

void SonyHttpCamera::begin(uint32_t ip)
{
    ip_addr = ip;
    state = SHCAMSTATE_CONNECTING;
    dd_tries = 0;
    error_cnt = 0;
    is_movierecording = false;
    rx_buff_idx = 0;
    rx_buff[0] = 0;
    last_poll_time = 0;
    if (state != SHCAMSTATE_FORBIDDEN || ip == 0) {
        state = SHCAMSTATE_NONE;
    }

    zoom_state = 0;
    zoom_time = 0;
    poll_delay = 500;

    if (iso_table != NULL) {
        free(iso_table);
    }
    if (shutterspd_table != NULL) {
        free(shutterspd_table);
    }

    if (ip != 0) {
        get_dd_xml();
    }
}

void SonyHttpCamera::parse_dd_xml(char* data, int32_t maxlen)
{
    int32_t i, j, k;
    int32_t slen = maxlen;
    if (maxlen <= 0) {
        slen = strlen(data);
    }
    bool hasName = false;
    char* keyFriendlyName = "<friendlyName>";
    int32_t keyLen = strlen(keyFriendlyName);
    for (i = 0; i < slen - keyLen; i++)
    {
        if (memcmp(&(data[i]), keyFriendlyName, keyLen) == 0)
        {
            printf("found friendly name idx %u\r\n", i);
            i += keyLen;
            for (k = 0; i < slen - keyLen; i++, k++)
            {
                char c = data[i];
                if (c == '<')
                {
                    friendlyName[k] = 0;
                    hasName = true;
                    break;
                }
                else
                {
                    friendlyName[k] = c;
                }
            }
            i += keyLen;
            break;
        }
    }

    bool hasUrl = false;
    char* keySrvTypeCam = "X_ScalarWebAPI_ServiceType>camera<";
    keyLen = strlen(keySrvTypeCam);
    for (; i < slen - keyLen && hasUrl == false; i++)
    {
        if (memcmp(&(data[i]), keySrvTypeCam, keyLen) == 0)
        {
            printf("found ServiceType idx %u\r\n", i);
            i += keyLen;
            char* keyActionListUrl = "ActionList_URL>http://";
            keyLen = strlen(keyActionListUrl);
            for (; i < slen - keyLen && hasUrl == false; i++)
            {
                if (memcmp(&(data[i]), keyActionListUrl, keyLen) == 0)
                {
                    printf("found ActionList_URL idx %u\r\n", i);
                    i += keyLen - 7;
                    for (k = 0; i < slen - keyLen; i++, k++)
                    {
                        char c = data[i];
                        if (c == '<')
                        {
                            sprintf((char*)(&serviceUrl[k]),"/camera");
                            hasUrl = true;
                            break;
                        }
                        else
                        {
                            serviceUrl[k] = c;
                        }
                    }
                }
            }
        }
    }

    if (hasUrl)
    {
        state = SHCAMSTATE_INIT_GOTDD;
    }
    else
    {
        state = SHCAMSTATE_FORBIDDEN;
    }
}

void SonyHttpCamera::get_dd_xml()
{
    sprintf(url_buffer, "http://%s:64321/dd.xml", IPAddress(ip_addr).toString().c_str());
    httpreq->onData(NULL, NULL);
    httpreq->onReadyStateChange(ddRequestCb, this);
    httpreq->open("GET", url_buffer);
    state = SHCAMSTATE_CONNECTING;
}

static void SonyHttpCamera::ddRequestCb(void* optParm, AsyncHTTPRequest* req, int readyState)
{
    SonyHttpCamera* cam = (SonyHttpCamera*)optParm;
    if (readyState == readyStateDone) 
    {
        if (req->responseHTTPcode() == 200)
        {
            // expect about 3 kilobytes of data
            cam->parse_dd_xml(req->responseLongText());
        }
        else
        {
            cam->dd_tries++;
            if (cam->dd_tries < 3)
            {
                cam->get_dd_xml();
            }
            else if (cam->state != SHCAMSTATE_FORBIDDEN)
            {
                cam->state = SHCAMSTATE_FAILED;
            }
        }
    }
}

void SonyHttpCamera::parse_event(char* data, int32_t maxlen)
{
    char res_buff[64] = {0};
    signed int i, j, k;
    bool found = scan_json_for_key(rx_buff, rx_buff_idx, "cameraStatus", &i, &j, (char*)res_buff, 64);
    if (found) {
        this->is_movierecording = (memcmp(res_buff, "Movie", 5) == 0);
    }
    found = scan_json_for_key(rx_buff, rx_buff_idx, "currentIsoSpeedRate"   , &i, &j, (char*)res_buff, 64);
    if (iso_table == NULL) {
        found = scan_json_for_key(rx_buff, rx_buff_idx, "isoSpeedRateCandidates", &i, &j, NULL, 0);
        if (found) {
            k = j - i + 3;
            iso_table = (char*)malloc(k);
            found = scan_json_for_key(rx_buff, rx_buff_idx, "isoSpeedRateCandidates", &i, &j, (char*)iso_table, k - 1);
        }
    }
    found = scan_json_for_key(rx_buff, rx_buff_idx, "currentShutterSpeed"   , &i, &j, (char*)res_buff, 64);
    if (shutterspd_table == NULL) {
        found = scan_json_for_key(rx_buff, rx_buff_idx, "shutterSpeedCandidates", &i, &j, NULL, 0);
        if (found) {
            k = j - i + 3;
            shutterspd_table = (char*)malloc(k);
            found = scan_json_for_key(rx_buff, rx_buff_idx, "shutterSpeedCandidates", &i, &j, (char*)shutterspd_table, k - 1);
        }
    }
}

static void SonyHttpCamera::eventRequestCb(void* optParm, AsyncHTTPRequest* req, int readyState)
{
    SonyHttpCamera* cam = (SonyHttpCamera*)optParm;
    if (req->available() <= 0) {
        return;
    }

    int32_t chunk;
    while ((chunk = req->available()) > 0)
    {
        int32_t r = read_in_chunk(cam, req, chunk);
        if (r == 0) {
            break;
        }
        cam->parse_event(cam->rx_buff, cam->rx_buff_idx);
    }

    if (readyState == readyStateDone)
    {
        if (req->responseHTTPcode() == 200) {
            error_cnt = 0;
        }
        else {
            error_cnt++;
        }
        if (state != SHCAMSTATE_FORBIDDEN) {
            state = SHCAMSTATE_READY;
        }
    }
}

static void SonyHttpCamera::eventDataCb(void* optParm, AsyncHTTPRequest* req, int avail)
{
    SonyHttpCamera* cam = (SonyHttpCamera*)optParm;
    if (req->available() <= 0) {
        return;
    }

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
    rx_buff_idx = 0;
    httpreq->onData(eventEventCb, this);
    httpreq->onReadyStateChange(eventRequestCb, this);
    httpreq->open("POST", url_buffer);
    httpreq->send("{\"method\": \"getEvent\", \"params\": [false], \"id\": 1, \"version\": \"1.0\"}");
    state = SHCAMSTATE_POLLING;
    last_poll_time = millis();
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

    if (state == SHCAMSTATE_INIT_STARTRECMODE || state == SHCAMSTATE_INIT_SETCAMFUNC)
    {
        rx_buff_idx = 0;
        httpreq->onData(NULL, NULL);
        httpreq->onReadyStateChange(initRequestCb, this);
        httpreq->open("POST", url_buffer);
        if (state == SHCAMSTATE_INIT_STARTRECMODE) {
            httpreq->send("{\"method\": \"startRecMode\", \"params\": [], \"id\": 1, \"version\": \"1.0\"}");
        }
        else if (state == SHCAMSTATE_INIT_SETCAMFUNC) {
            httpreq->send("{\"method\": \"cameraFunction\", \"params\": [\"Remote Shooting\"], \"id\": 1, \"version\": \"1.0\"}");
        }
        state++; // set the flag for waiting
        last_poll_time = millis();
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
    }

    if (error_cnt > 10) {
        state = SHCAMSTATE_FAILED;
    }
}

void SonyHttpCamera::wait_while_busy(uint32_t min_time, uint32_t max_time, volatile bool* exit_signal)
{
    volatile bool to_exit = false;
    uint32_t start_time = millis();
    uint32_t now = start_time;
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
}

static void SonyHttpCamera::initRequestCb(void* optParm, AsyncHTTPRequest* req, int readyState)
{
    SonyHttpCamera* cam = (SonyHttpCamera*)optParm;
    if (readyState == readyStateDone) 
    {
        int rcode;
        int do_next = 0;
        if ((rcode = req->responseHTTPcode()) == 200)
        {
            do_next = cam->state + 1; // go to next step
            cam->dd_tries = 0;
        }
        else
        {
            cam->dd_tries++;
            if (cam->dd_tries < 3)
            {
                do_next = cam->state - 1; // retry previous step
            }
            else if (cam->state != SHCAMSTATE_FORBIDDEN)
            {
                cam->state = SHCAMSTATE_FAILED;
            }
        }
        if (do_next != 0 && cam->state != SHCAMSTATE_FORBIDDEN)
        {
            cam->state = do_next; // next call to poll will handle it
        }
    }
}

static void SonyHttpCamera::genericRequestCb(void* optParm, AsyncHTTPRequest* req, int readyState)
{
    SonyHttpCamera* cam = (SonyHttpCamera*)optParm;
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
    }
}
