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
    bool openres = request_prep("GET", url_buffer, NULL, ddRequestCb, NULL);
    if (openres) {
        httpreq->send();
        state = SHCAMSTATE_INIT_GETDD;
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