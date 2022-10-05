#ifndef _ALPHAFAIRYCAMERA_H_
#define _ALPHAFAIRYCAMERA_H_

// this class exists to make the main application code neater
// it encapsulates the PtpIpSonyAlphaCamera and SonyHttpCamera class
// so that common functions can be called upon the AlphaFairyCamera class
// and the AlphaFairyCamera class decides which protocol to send the command with, depending on how the camera is actually connected

#include <PtpIpSonyAlphaCamera.h>
#include <SonyHttpCamera.h>

class AlphaFairyCamera
{
    public:
        AlphaFairyCamera(PtpIpSonyAlphaCamera* cam_p, SonyHttpCamera* cam_h) { cam_ptp = cam_p; cam_http = cam_h; };
    private:
        PtpIpSonyAlphaCamera* cam_ptp;
        SonyHttpCamera*       cam_http;
    public:
        uint32_t  getIp           (void);
        char*     getCameraName   (void);
        void wait_while_busy(uint32_t min_wait, uint32_t max_wait, volatile bool* exit_signal = NULL);
        inline bool isOperating() { return cam_ptp->isOperating() || cam_http->isOperating(); };
        bool is_movierecording(void);
        bool is_manuallyfocused(void);
        uint32_t get_exposureMode(void);
        bool need_wait_af(void);
        bool is_focused(void);
        bool cmd_AutoFocus(bool onoff);
        bool cmd_Shutter(bool openclose);
        bool cmd_FocusPointSet(int16_t x, int16_t y);
        bool cmd_Shoot(int t);
        bool cmd_MovieRecord(bool onoff);
        bool cmd_MovieRecordToggle();
        bool cmd_ManualFocusMode(bool onoff, bool precheck = false);
        bool cmd_ManualFocusToggle(bool onoff);
        bool cmd_IsoSet(uint32_t x);
        void set_debugflags(uint32_t x);
};

#endif
