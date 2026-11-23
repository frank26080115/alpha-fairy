// Copy to PersonalPreloadedProfile.ino (ignored by Git) and customize the placeholders.
// In your copy, replace #if 0 with:
// #if defined(ADD_PERSONAL_PRELOADED_PROFILE) && ADD_PERSONAL_PRELOADED_PROFILE
// Select a -personal PlatformIO environment to enable the hooks.

#if 0

#include "AlphaFairy.h"
#include <string.h>

void settings_default_personal(void)
{
    // Pacific standard time (UTC-8). Change to TIMEZONE_PDT (UTC-7) in summer.
    // This overrides the compiler-host timezone fallback when defaults are loaded.
    config_settings.time_utc_offset_minutes = TIMEZONE_PST;
    // Example default override:
    // config_settings.mictrig_level = 74;
}

void preload_first_wifi_profile(void)
{
    // Called by settings_default() only when the first saved slot is empty.
    wifiprofile_t profile = {};
    strcpy(profile.ssid, "YOUR_CAMERA_SSID");
    strcpy(profile.password, "YOUR_CAMERA_PASSWORD");
    profile.opmode = WIFIOPMODE_STA;
    wifiprofile_writeProfile(1, &profile);
}

#endif
