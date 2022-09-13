#include "AlphaFairy.h"

extern bool autoconnect_active;
extern int autoconnect_status;
extern bool signal_wifiauthfailed;

void cam_cb_setup()
{
    ptpcam.cb_onConnect = ptpcam_onConnect;
    ptpcam.cb_onDisconnect = ptpcam_onDisconnect;
    ptpcam.cb_onCriticalError = ptpcam_onCriticalError;
    ptpcam.cb_onReject = ptpcam_onReject;
    ptpcam.cb_onConfirmedAvail = ptpcam_onConfirmedAvail;

    httpcam.cb_onConnect = ptpcam_onConnect;
    httpcam.cb_onDisconnect = httpcam_onDisconnect;
    httpcam.cb_onCriticalError = httpcam_onCriticalError;
}

void wifi_onConnect()
{
    dbg_ser.printf("application wifi event handler called\r\n");
    autoconnect_status = AUTOCONNSTS_CONNECTED;
    pwr_tick(true);
    if (ptpcam.canNewConnect()) {
        uint32_t newip = NetMgr_getConnectableClient();
        if (newip != 0)
        {
            #if 1
            if (NetMgr_getOpMode() == WIFIOPMODE_STA)
            {
                // if the mode is STA then it is more likely that the camera is HTTP
                // it seems like some of those cameras do not like being connected to over the PTP port first
                // so add an extra delay for the PTP initialization just in case
                ptpcam.begin(newip, 5000);
            }
            else
            #endif
            {
                ptpcam.begin(newip);
            }
            wifiprofile_t wp;
            bool has_profile = wifiprofile_getProfile(config_settings.wifi_profile, &wp);
            ptpcam.install_guid((config_settings.wifi_profile == 0 || has_profile == false) ? NULL : wp.guid);
        }
    }
    if (httpcam.canNewConnect()) {
        uint32_t newip = NetMgr_getConnectableClient();
        if (newip != 0) {
            httpcam.begin(newip);
        }
    }
}

void wifi_onDisconnect(uint8_t x, int reason)
{
    if (x == WIFIDISCON_NORMAL)
    {
        Serial.printf("WiFi disconnected normal, reason %d\r\n", reason);
        if (httpcam.getState() != 0) {
            httpcam.begin(0); // this resets the forbidden flag so it can reconnect again later 
        }
    }
    else if (x == WIFIDISCON_AUTH_ERROR)
    {
        Serial.printf("WiFi disconnected error, reason %d\r\n", reason);
        wifi_err_reason = reason;
        critical_error("/wifi_error.png");
    }
    else if (x == WIFIDISCON_AUTH_FAIL)
    {
        Serial.printf("WiFi disconnected auth failed, reason %d\r\n", reason);
        wifi_err_reason = reason;
        autoconnect_status = AUTOCONNSTS_FAILED;
        if (autoconnect_active == false) {
            //force_wifi_config("/wifi_reject.png");
            WiFi.disconnect(); // prevent reconnection attempt
            signal_wifiauthfailed = true; // signal to the GUI thread to show error
        }
    }
}

void ptpcam_onConnect()
{
    dbg_ser.printf("ptpcam_onConnect\r\n");
    pwr_tick(true);
    if (ptpcam.isOperating()) {
        NetMgr_markClientCameraPtp(ptpcam.getIp());
        httpcam.setForbidden();
    }
}

void httpcam_onConnect()
{
    dbg_ser.printf("httpcam_onConnect\r\n");
    pwr_tick(true);
    if (httpcam.isOperating()) {
        NetMgr_markClientCameraHttp(httpcam.getIp());
    }
}

void ptpcam_onDisconnect()
{
    pwr_tick(true);
    NetMgr_markClientDisconnect(ptpcam.getIp());
}

void httpcam_onDisconnect()
{
    pwr_tick(true);
    NetMgr_markClientDisconnect(httpcam.getIp());
}

void ptpcam_onCriticalError()
{
    pwr_tick(true);
    if (ptpcam.critical_error_cnt > 2) {
        NetMgr_markClientError(ptpcam.getIp());
        if (NetMgr_shouldReportError() && httpcam.isOperating() == false) {
            critical_error("/crit_error.png");
        }
    }
}

void httpcam_onCriticalError()
{
    pwr_tick(true);
    if (httpcam.critical_error_cnt > 0 && NetMgr_getOpMode() == WIFIOPMODE_STA) {
        NetMgr_markClientError(httpcam.getIp());
        if (NetMgr_shouldReportError() && ptpcam.isOperating() == false) {
            critical_error("/crit_error.png");
        }
    }
}

void ptpcam_onReject()
{
    critical_error("/rejected.png");
}

void ptpcam_onConfirmedAvail()
{
    httpcam.setForbidden();
}