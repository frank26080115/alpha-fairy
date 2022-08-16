#include "SonyHttpCamera.h"

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
                            strcpy(access_url, service_url);
                            sprintf((char*)(&service_url[k]),"/camera");
                            sprintf((char*)(&access_url[k]),"/accessControl");
                            hasUrl = true;
                            break;
                        }
                        else
                        {
                            service_url[k] = c;
                            service_url[k + 1] = 0;
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
    if (strlen(url_buffer) <= 0) {
        sprintf(url_buffer, "http://%s:64321/dd.xml", IPAddress(ip_addr).toString().c_str());
    }
    dbgser_states->printf("getting dd.xml from URL: %s\r\n", url_buffer);
    #ifdef SHCAM_USE_ASYNC
    bool openres = request_prep("GET", url_buffer, NULL, ddRequestCb, NULL);
    if (openres) {
        httpreq->send();
        state = SHCAMSTATE_INIT_GETDD;
    }
    #else
    httpclient.begin(url_buffer);
    last_http_resp_code = httpclient.GET();
    http_content_len = httpclient.getSize();
    if (last_http_resp_code == 200)
    {
        dbgser_states->printf("httpcam success got dd.xml\r\n");
        parse_dd_xml((char*)httpclient.getString().c_str());
    }
    #endif
    else {
        dbgser_states->printf("httpcam unable to get dd.xml");
        #ifndef SHCAM_USE_ASYNC
        init_retries++;
        if (init_retries > 3) {
            dbgser_states->printf(", resp code %d, giving up", last_http_resp_code);
            state = SHCAMSTATE_FORBIDDEN;
        }
        else {
            dbgser_states->printf(", resp code %d, try %u", last_http_resp_code, init_retries);
            state = SHCAMSTATE_INIT_GETDD;
        }
        #endif
        dbgser_states->printf("\r\n");
    }
}

#ifdef SHCAM_USE_ASYNC
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
            cam->init_retries++;
            if (cam->init_retries < 3) {
                dbgser_states->printf(", retrying (%u)\r\n", cam->init_retries);
                cam->get_dd_xml();
            }
            else {
                dbgser_states->printf(", giving up (%u)\r\n", cam->init_retries);
                cam->state = SHCAMSTATE_FAILED;
            }
        }
        cam->request_close();
    }
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
            cam->init_retries = 0;
        }
        else
        {
            dbgser_states->printf("httpcam init state %u error %d", cam->state, rcode);
            cam->init_retries++;
            if (cam->init_retries < 2) {
                dbgser_states->printf(", retry %d\r\n", cam->init_retries);
                do_next = cam->state - 1; // retry previous step
            }
            else {
                dbgser_states->printf(", skipping %d, do next\r\n", cam->init_retries);
                //cam->state = SHCAMSTATE_FAILED;
                do_next = cam->state + 1;
                cam->init_retries = 0;
            }
        }
        if (do_next != 0)
        {
            cam->state = do_next; // next call to poll will handle it
        }
        cam->request_close();
    }
}
#endif

void SonyHttpCamera::ssdp_start(void)
{
    ssdp_udp.beginMulticast(IPAddress(239,255,255,250), 1900);
    ssdp_udp.beginMulticastPacket();
    ssdp_udp.print("M-SEARCH * HTTP/1.1\r\n");
    ssdp_udp.print("HOST: 239.255.255.250:1900\r\n");
    ssdp_udp.print("MAN: \"ssdp:discover\"\r\n");
    ssdp_udp.print("MX: 2\r\n");
    ssdp_udp.print("ST: urn:schemas-sony-com:service:ScalarWebAPI:1\r\n");
    ssdp_udp.print("USER-AGENT: xyz/1.0 abc/1.0\r\n");
    ssdp_udp.print("\r\n");
    ssdp_udp.endPacket();
}

bool SonyHttpCamera::ssdp_checkurl(void)
{
    String s;
    while (ssdp_udp.available())
    {
        s = ssdp_udp.readStringUntil('\n');
        if (s.startsWith("LOCATION:"))
        {
            char* ss = (char*)&(s.c_str()[9]);
            int i, j;
            for (i = 0; ; i++) {
                char c = ss[i];
                if (c == 'h') {
                    for (j = 0; ; i++, j++) {
                        c = ss[i];
                        if (c == '\r' || c == '\n' || c == 0) {
                            return true;
                        }
                        url_buffer[j] = c;
                        url_buffer[j + 1] = 0;
                    }
                }
            }
        }
    }
    return false;
}
