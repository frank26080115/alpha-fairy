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
        if (cb_onNoServiceUrl != NULL) {
            cb_onNoServiceUrl();
        }
    }
}

void SonyHttpCamera::get_dd_xml()
{
    char* url_buffer_used = (char*)cmd_buffer; // borrow another buffer
    if (strlen(url_buffer) <= 0)
    {
        // we didn't get a valid URL from SSDP
        // try different default URLs until something works
        uint8_t rem = init_retries % 2;
        if (rem == 0) {
            sprintf(url_buffer_used, "http://%s:64321/dd.xml", IPAddress(ip_addr).toString().c_str());
        }
        else if (rem == 1) {
            sprintf(url_buffer_used, "http://%s:61000/scalarwebapi_dd.xml", IPAddress(ip_addr).toString().c_str());
        }
    }
    else
    {
        strcpy(url_buffer_used, url_buffer);
    }
    dbgser_states->printf("getting dd.xml from URL: %s\r\n", url_buffer_used);
    httpclient.begin(url_buffer_used);
    last_http_resp_code = httpclient.GET();
    http_content_len = httpclient.getSize();
    if (last_http_resp_code == 200)
    {
        dbgser_states->printf("httpcam success got dd.xml\r\n");
        parse_dd_xml((char*)httpclient.getString().c_str());
    }
    else {
        dbgser_states->printf("httpcam unable to get dd.xml");
        init_retries++;
        if (init_retries > 10) {
            critical_error_cnt++;
            dbgser_states->printf(", resp code %d, giving up", last_http_resp_code);
            state = SHCAMSTATE_FORBIDDEN;
            if (cb_onCriticalError != NULL) {
                cb_onCriticalError();
            }
        }
        else {
            dbgser_states->printf(", resp code %d, try %u", last_http_resp_code, init_retries);
            state = SHCAMSTATE_INIT_GETDD;
        }
        dbgser_states->printf("\r\n");
    }
}

void SonyHttpCamera::ssdp_start(WiFiUDP* sock)
{
    if (sock == NULL) {
        return;
    }
    sock->beginMulticast(IPAddress(239,255,255,250), 1900);
    sock->beginMulticastPacket();
    sock->print("M-SEARCH * HTTP/1.1\r\n");
    sock->print("HOST: 239.255.255.250:1900\r\n");
    sock->print("MAN: \"ssdp:discover\"\r\n");

    // cycle through possible queries
    uint8_t rem = init_retries % 4;
    if (rem == 0 || rem == 2) {
        sock->print("MX: 1\r\n");
        sock->print("ST: urn:schemas-sony-com:service:ScalarWebAPI:1\r\n");
        sock->print("USER-AGENT: xyz/1.0 abc/1.0\r\n");
    }
    else if (rem == 1) {
        sock->print("MX: 1\r\n");
        sock->print("ST: urn:dial-multiscreen-org:service:dial:1\r\n");
        sock->print("USER-AGENT: Google Chrome/1.0 Windows\r\n");
    }
    else if (rem == 3) {
        sock->print("ST:ssdp:all\r\n");
        sock->print("MX: 1\r\n");
    }

    sock->print("\r\n");
    sock->endPacket();
}

bool SonyHttpCamera::ssdp_checkurl(WiFiUDP* sock)
{
    if (sock == NULL) {
        return false;
    }
    String s;
    while (sock->available())
    {
        s = sock->readStringUntil('\n');
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

bool SonyHttpCamera::ssdp_poll(WiFiUDP* sock)
{
    bool got_ssdp = false;
    bool got_url_ssdp = false;
    uint32_t now = millis();

    if (sock == NULL) {
        return false;
    }

    if (now >= wait_until)
    {
        sock->parsePacket();
        if (sock->available() > 0) {
            got_url_ssdp = ssdp_checkurl(sock);
            got_ssdp |= got_url_ssdp;
            dbgser_states->print("httpcam SSDP got reply, reached time to parse");
            if (got_ssdp) {
                dbgser_states->printf(", URL: %s\r\n", url_buffer);
            }
            else {
                dbgser_states->print(", no URL result\r\n");
            }
        }

        if (got_ssdp == false) {
            init_retries++;
            if ((now - start_time) >= (ssdp_allowed_time * 1000)) {
                got_ssdp = true; // give up and get dd.xml anyways
                critical_error_cnt++;
                dbgser_states->println("httpcam SSDP give up");
            }
            else {
                dbgser_states->printf("httpcam SSDP timeout, try %u\r\n", init_retries);
                ssdp_start(sock);
                wait_until = now + 250;
            }
        }
    }
    if (got_ssdp) {
        init_retries = 0;
        get_dd_xml();
    }
    return got_url_ssdp;
}
