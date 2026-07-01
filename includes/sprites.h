#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(ARDUINO)
#include <pgmspace.h>
#endif

#ifndef PROGMEM
#define PROGMEM
#endif

#define SPRITE_9POINT_UNABLE_WIDTH 135u
#define SPRITE_9POINT_UNABLE_HEIGHT 240u
#define SPRITE_9POINT_UNABLE_BYTES 2675u
extern const uint8_t sprite_9point_unable[];

#define SPRITE_ABOUT_WIDTH 135u
#define SPRITE_ABOUT_HEIGHT 240u
#define SPRITE_ABOUT_BYTES 13985u
extern const uint8_t sprite_about[];

#define SPRITE_AUTOCONN_ICON_WIDTH 60u
#define SPRITE_AUTOCONN_ICON_HEIGHT 60u
#define SPRITE_AUTOCONN_ICON_BYTES 4320u
extern const uint8_t sprite_autoconn_icon[];

#define SPRITE_BACK_WIDTH 135u
#define SPRITE_BACK_HEIGHT 240u
#define SPRITE_BACK_BYTES 4110u
extern const uint8_t sprite_back[];

#define SPRITE_BACK_ICON_WIDTH 60u
#define SPRITE_BACK_ICON_HEIGHT 60u
#define SPRITE_BACK_ICON_BYTES 1875u
extern const uint8_t sprite_back_icon[];

#define SPRITE_BATTFULL_BLACK_WIDTH 135u
#define SPRITE_BATTFULL_BLACK_HEIGHT 12u
#define SPRITE_BATTFULL_BLACK_BYTES 658u
extern const uint8_t sprite_battfull_black[];

#define SPRITE_BATTFULL_WHITE_WIDTH 135u
#define SPRITE_BATTFULL_WHITE_HEIGHT 12u
#define SPRITE_BATTFULL_WHITE_BYTES 1058u
extern const uint8_t sprite_battfull_white[];

#define SPRITE_CAMERA_ICON_WIDTH 60u
#define SPRITE_CAMERA_ICON_HEIGHT 60u
#define SPRITE_CAMERA_ICON_BYTES 4037u
extern const uint8_t sprite_camera_icon[];

#define SPRITE_CHARGING_BLACK_WIDTH 135u
#define SPRITE_CHARGING_BLACK_HEIGHT 12u
#define SPRITE_CHARGING_BLACK_BYTES 841u
extern const uint8_t sprite_charging_black[];

#define SPRITE_CHARGING_WHITE_WIDTH 135u
#define SPRITE_CHARGING_WHITE_HEIGHT 12u
#define SPRITE_CHARGING_WHITE_BYTES 1068u
extern const uint8_t sprite_charging_white[];

#define SPRITE_CONFIG_WIDTH 135u
#define SPRITE_CONFIG_HEIGHT 240u
#define SPRITE_CONFIG_BYTES 17567u
extern const uint8_t sprite_config[];

#define SPRITE_CONFIG_ICON_WIDTH 60u
#define SPRITE_CONFIG_ICON_HEIGHT 60u
#define SPRITE_CONFIG_ICON_BYTES 4512u
extern const uint8_t sprite_config_icon[];

#define SPRITE_CONNECTING0_WIDTH 135u
#define SPRITE_CONNECTING0_HEIGHT 240u
#define SPRITE_CONNECTING0_BYTES 13664u
extern const uint8_t sprite_connecting0[];

#define SPRITE_CONNECTING1_WIDTH 135u
#define SPRITE_CONNECTING1_HEIGHT 240u
#define SPRITE_CONNECTING1_BYTES 13408u
extern const uint8_t sprite_connecting1[];

#define SPRITE_CRIT_ERROR_WIDTH 135u
#define SPRITE_CRIT_ERROR_HEIGHT 240u
#define SPRITE_CRIT_ERROR_BYTES 4478u
extern const uint8_t sprite_crit_error[];

#define SPRITE_DANCE_ICON_WIDTH 60u
#define SPRITE_DANCE_ICON_HEIGHT 60u
#define SPRITE_DANCE_ICON_BYTES 4358u
extern const uint8_t sprite_dance_icon[];

#define SPRITE_DEAD_BATT_WIDTH 135u
#define SPRITE_DEAD_BATT_HEIGHT 240u
#define SPRITE_DEAD_BATT_BYTES 4794u
extern const uint8_t sprite_dead_batt[];

#define SPRITE_DUALSHUTTER_REG_WIDTH 135u
#define SPRITE_DUALSHUTTER_REG_HEIGHT 240u
#define SPRITE_DUALSHUTTER_REG_BYTES 9741u
extern const uint8_t sprite_dualshutter_reg[];

#define SPRITE_DUALSHUTTER_SHOOT_WIDTH 135u
#define SPRITE_DUALSHUTTER_SHOOT_HEIGHT 240u
#define SPRITE_DUALSHUTTER_SHOOT_BYTES 9517u
extern const uint8_t sprite_dualshutter_shoot[];

#define SPRITE_EXTINPUT_ICON_WIDTH 60u
#define SPRITE_EXTINPUT_ICON_HEIGHT 60u
#define SPRITE_EXTINPUT_ICON_BYTES 1293u
extern const uint8_t sprite_extinput_icon[];

#define SPRITE_FOCUS_9POINT_WIDTH 135u
#define SPRITE_FOCUS_9POINT_HEIGHT 240u
#define SPRITE_FOCUS_9POINT_BYTES 3044u
extern const uint8_t sprite_focus_9point[];

#define SPRITE_FOCUS_CALIB_WIDTH 135u
#define SPRITE_FOCUS_CALIB_HEIGHT 240u
#define SPRITE_FOCUS_CALIB_BYTES 8054u
extern const uint8_t sprite_focus_calib[];

#define SPRITE_FOCUS_FRUST_WIDTH 135u
#define SPRITE_FOCUS_FRUST_HEIGHT 240u
#define SPRITE_FOCUS_FRUST_BYTES 25032u
extern const uint8_t sprite_focus_frust[];

#define SPRITE_FOCUS_PULL_WIDTH 135u
#define SPRITE_FOCUS_PULL_HEIGHT 240u
#define SPRITE_FOCUS_PULL_BYTES 5578u
extern const uint8_t sprite_focus_pull[];

#define SPRITE_FOCUSSTACK_WIDTH 135u
#define SPRITE_FOCUSSTACK_HEIGHT 240u
#define SPRITE_FOCUSSTACK_BYTES 9932u
extern const uint8_t sprite_focusstack[];

#define SPRITE_FOCUSSTACK_FAR_1_WIDTH 135u
#define SPRITE_FOCUSSTACK_FAR_1_HEIGHT 240u
#define SPRITE_FOCUSSTACK_FAR_1_BYTES 9060u
extern const uint8_t sprite_focusstack_far_1[];

#define SPRITE_FOCUSSTACK_FAR_2_WIDTH 135u
#define SPRITE_FOCUSSTACK_FAR_2_HEIGHT 240u
#define SPRITE_FOCUSSTACK_FAR_2_BYTES 9047u
extern const uint8_t sprite_focusstack_far_2[];

#define SPRITE_FPULL_P0_WIDTH 135u
#define SPRITE_FPULL_P0_HEIGHT 24u
#define SPRITE_FPULL_P0_BYTES 469u
extern const uint8_t sprite_fpull_p0[];

#define SPRITE_GALAXY_ICON_WIDTH 60u
#define SPRITE_GALAXY_ICON_HEIGHT 60u
#define SPRITE_GALAXY_ICON_BYTES 5696u
extern const uint8_t sprite_galaxy_icon[];

#define SPRITE_GO_ICON_WIDTH 60u
#define SPRITE_GO_ICON_HEIGHT 60u
#define SPRITE_GO_ICON_BYTES 3524u
extern const uint8_t sprite_go_icon[];

#define SPRITE_IMU_ICON_WIDTH 60u
#define SPRITE_IMU_ICON_HEIGHT 60u
#define SPRITE_IMU_ICON_BYTES 1627u
extern const uint8_t sprite_imu_icon[];

#define SPRITE_INTERVALOMETER_WIDTH 60u
#define SPRITE_INTERVALOMETER_HEIGHT 60u
#define SPRITE_INTERVALOMETER_BYTES 2142u
extern const uint8_t sprite_intervalometer[];

#define SPRITE_INTERVALOMETER_ICON_WIDTH 60u
#define SPRITE_INTERVALOMETER_ICON_HEIGHT 60u
#define SPRITE_INTERVALOMETER_ICON_BYTES 2142u
extern const uint8_t sprite_intervalometer_icon[];

#define SPRITE_LEPTON_WIDTH 135u
#define SPRITE_LEPTON_HEIGHT 240u
#define SPRITE_LEPTON_BYTES 21724u
extern const uint8_t sprite_lepton[];

#define SPRITE_LEPTON_ICON_WIDTH 60u
#define SPRITE_LEPTON_ICON_HEIGHT 60u
#define SPRITE_LEPTON_ICON_BYTES 1825u
extern const uint8_t sprite_lepton_icon[];

#define SPRITE_LOWBATT_BLACK_WIDTH 135u
#define SPRITE_LOWBATT_BLACK_HEIGHT 12u
#define SPRITE_LOWBATT_BLACK_BYTES 599u
extern const uint8_t sprite_lowbatt_black[];

#define SPRITE_LOWBATT_WHITE_WIDTH 135u
#define SPRITE_LOWBATT_WHITE_HEIGHT 12u
#define SPRITE_LOWBATT_WHITE_BYTES 757u
extern const uint8_t sprite_lowbatt_white[];

#define SPRITE_MAIN_ALL_WIDTH 135u
#define SPRITE_MAIN_ALL_HEIGHT 240u
#define SPRITE_MAIN_ALL_BYTES 22213u
extern const uint8_t sprite_main_all[];

#define SPRITE_MAIN_ASTRO_WIDTH 135u
#define SPRITE_MAIN_ASTRO_HEIGHT 240u
#define SPRITE_MAIN_ASTRO_BYTES 17489u
extern const uint8_t sprite_main_astro[];

#define SPRITE_MAIN_AUTO_WIDTH 135u
#define SPRITE_MAIN_AUTO_HEIGHT 240u
#define SPRITE_MAIN_AUTO_BYTES 14623u
extern const uint8_t sprite_main_auto[];

#define SPRITE_MAIN_FOCUS_WIDTH 135u
#define SPRITE_MAIN_FOCUS_HEIGHT 240u
#define SPRITE_MAIN_FOCUS_BYTES 15436u
extern const uint8_t sprite_main_focus[];

#define SPRITE_MAIN_INTERVAL_WIDTH 135u
#define SPRITE_MAIN_INTERVAL_HEIGHT 240u
#define SPRITE_MAIN_INTERVAL_BYTES 15503u
extern const uint8_t sprite_main_interval[];

#define SPRITE_MAIN_LEPTON_WIDTH 135u
#define SPRITE_MAIN_LEPTON_HEIGHT 240u
#define SPRITE_MAIN_LEPTON_BYTES 13101u
extern const uint8_t sprite_main_lepton[];

#define SPRITE_MAIN_REMOTE_WIDTH 135u
#define SPRITE_MAIN_REMOTE_HEIGHT 240u
#define SPRITE_MAIN_REMOTE_BYTES 14850u
extern const uint8_t sprite_main_remote[];

#define SPRITE_MAIN_UTILS_WIDTH 135u
#define SPRITE_MAIN_UTILS_HEIGHT 240u
#define SPRITE_MAIN_UTILS_BYTES 15848u
extern const uint8_t sprite_main_utils[];

#define SPRITE_MIC_ICON_WIDTH 60u
#define SPRITE_MIC_ICON_HEIGHT 60u
#define SPRITE_MIC_ICON_BYTES 2221u
extern const uint8_t sprite_mic_icon[];

#define SPRITE_NO_CAMERA_BLACK_WIDTH 135u
#define SPRITE_NO_CAMERA_BLACK_HEIGHT 12u
#define SPRITE_NO_CAMERA_BLACK_BYTES 613u
extern const uint8_t sprite_no_camera_black[];

#define SPRITE_NO_CAMERA_WHITE_WIDTH 135u
#define SPRITE_NO_CAMERA_WHITE_HEIGHT 12u
#define SPRITE_NO_CAMERA_WHITE_BYTES 804u
extern const uint8_t sprite_no_camera_white[];

#define SPRITE_PAIR_REJECT_WIDTH 135u
#define SPRITE_PAIR_REJECT_HEIGHT 240u
#define SPRITE_PAIR_REJECT_BYTES 18385u
extern const uint8_t sprite_pair_reject[];

#define SPRITE_QIKRMT_ACTIVE_WIDTH 135u
#define SPRITE_QIKRMT_ACTIVE_HEIGHT 240u
#define SPRITE_QIKRMT_ACTIVE_BYTES 12454u
extern const uint8_t sprite_qikrmt_active[];

#define SPRITE_QIKRMT_FADED_WIDTH 135u
#define SPRITE_QIKRMT_FADED_HEIGHT 240u
#define SPRITE_QIKRMT_FADED_BYTES 10673u
extern const uint8_t sprite_qikrmt_faded[];

#define SPRITE_RECORDMOVIE_WIDTH 135u
#define SPRITE_RECORDMOVIE_HEIGHT 240u
#define SPRITE_RECORDMOVIE_BYTES 8531u
extern const uint8_t sprite_recordmovie[];

#define SPRITE_REMOTESHUTTER_WIDTH 135u
#define SPRITE_REMOTESHUTTER_HEIGHT 240u
#define SPRITE_REMOTESHUTTER_BYTES 13681u
extern const uint8_t sprite_remoteshutter[];

#define SPRITE_REMOTESHUTTER_D_WIDTH 135u
#define SPRITE_REMOTESHUTTER_D_HEIGHT 240u
#define SPRITE_REMOTESHUTTER_D_BYTES 14707u
extern const uint8_t sprite_remoteshutter_d[];

#define SPRITE_SHUTTER_STEP_WIDTH 135u
#define SPRITE_SHUTTER_STEP_HEIGHT 240u
#define SPRITE_SHUTTER_STEP_BYTES 14697u
extern const uint8_t sprite_shutter_step[];

#define SPRITE_SHUTTERTRIGGER_WIDTH 135u
#define SPRITE_SHUTTERTRIGGER_HEIGHT 240u
#define SPRITE_SHUTTERTRIGGER_BYTES 14498u
extern const uint8_t sprite_shuttertrigger[];

#define SPRITE_SLEEP_WIDTH 135u
#define SPRITE_SLEEP_HEIGHT 240u
#define SPRITE_SLEEP_BYTES 6671u
extern const uint8_t sprite_sleep[];

#define SPRITE_SOUNDSHUTTER_WIDTH 135u
#define SPRITE_SOUNDSHUTTER_HEIGHT 240u
#define SPRITE_SOUNDSHUTTER_BYTES 12844u
extern const uint8_t sprite_soundshutter[];

#define SPRITE_SPLASH_WIDTH 135u
#define SPRITE_SPLASH_HEIGHT 240u
#define SPRITE_SPLASH_BYTES 13181u
extern const uint8_t sprite_splash[];

#define SPRITE_STATUS_AIRPLANE_BLACK_WIDTH 32u
#define SPRITE_STATUS_AIRPLANE_BLACK_HEIGHT 12u
#define SPRITE_STATUS_AIRPLANE_BLACK_BYTES 197u
extern const uint8_t sprite_status_airplane_black[];

#define SPRITE_STATUS_AIRPLANE_WHITE_WIDTH 32u
#define SPRITE_STATUS_AIRPLANE_WHITE_HEIGHT 12u
#define SPRITE_STATUS_AIRPLANE_WHITE_BYTES 208u
extern const uint8_t sprite_status_airplane_white[];

#define SPRITE_STATUS_CHARGING_BLACK_WIDTH 32u
#define SPRITE_STATUS_CHARGING_BLACK_HEIGHT 12u
#define SPRITE_STATUS_CHARGING_BLACK_BYTES 241u
extern const uint8_t sprite_status_charging_black[];

#define SPRITE_STATUS_CHARGING_WHITE_WIDTH 32u
#define SPRITE_STATUS_CHARGING_WHITE_HEIGHT 12u
#define SPRITE_STATUS_CHARGING_WHITE_BYTES 316u
extern const uint8_t sprite_status_charging_white[];

#define SPRITE_STATUS_CHGLOW_BLACK_WIDTH 32u
#define SPRITE_STATUS_CHGLOW_BLACK_HEIGHT 12u
#define SPRITE_STATUS_CHGLOW_BLACK_BYTES 240u
extern const uint8_t sprite_status_chglow_black[];

#define SPRITE_STATUS_CHGLOW_WHITE_WIDTH 32u
#define SPRITE_STATUS_CHGLOW_WHITE_HEIGHT 12u
#define SPRITE_STATUS_CHGLOW_WHITE_BYTES 269u
extern const uint8_t sprite_status_chglow_white[];

#define SPRITE_STATUS_FULLBATT_BLACK_WIDTH 32u
#define SPRITE_STATUS_FULLBATT_BLACK_HEIGHT 12u
#define SPRITE_STATUS_FULLBATT_BLACK_BYTES 138u
extern const uint8_t sprite_status_fullbatt_black[];

#define SPRITE_STATUS_FULLBATT_WHITE_WIDTH 32u
#define SPRITE_STATUS_FULLBATT_WHITE_HEIGHT 12u
#define SPRITE_STATUS_FULLBATT_WHITE_BYTES 157u
extern const uint8_t sprite_status_fullbatt_white[];

#define SPRITE_STATUS_LOWBATT_BLACK_WIDTH 32u
#define SPRITE_STATUS_LOWBATT_BLACK_HEIGHT 12u
#define SPRITE_STATUS_LOWBATT_BLACK_BYTES 163u
extern const uint8_t sprite_status_lowbatt_black[];

#define SPRITE_STATUS_LOWBATT_WHITE_WIDTH 32u
#define SPRITE_STATUS_LOWBATT_WHITE_HEIGHT 12u
#define SPRITE_STATUS_LOWBATT_WHITE_BYTES 189u
extern const uint8_t sprite_status_lowbatt_white[];

#define SPRITE_STATUS_NOCAM_BLACK_WIDTH 32u
#define SPRITE_STATUS_NOCAM_BLACK_HEIGHT 12u
#define SPRITE_STATUS_NOCAM_BLACK_BYTES 248u
extern const uint8_t sprite_status_nocam_black[];

#define SPRITE_STATUS_NOCAM_WHITE_WIDTH 32u
#define SPRITE_STATUS_NOCAM_WHITE_HEIGHT 12u
#define SPRITE_STATUS_NOCAM_WHITE_BYTES 270u
extern const uint8_t sprite_status_nocam_white[];

#define SPRITE_TIMECODE_RESET_WIDTH 135u
#define SPRITE_TIMECODE_RESET_HEIGHT 240u
#define SPRITE_TIMECODE_RESET_BYTES 9986u
extern const uint8_t sprite_timecode_reset[];

#define SPRITE_TIMER_0_WIDTH 60u
#define SPRITE_TIMER_0_HEIGHT 60u
#define SPRITE_TIMER_0_BYTES 2007u
extern const uint8_t sprite_timer_0[];

#define SPRITE_TIMER_1_WIDTH 60u
#define SPRITE_TIMER_1_HEIGHT 60u
#define SPRITE_TIMER_1_BYTES 1975u
extern const uint8_t sprite_timer_1[];

#define SPRITE_TIMER_10_WIDTH 60u
#define SPRITE_TIMER_10_HEIGHT 60u
#define SPRITE_TIMER_10_BYTES 1995u
extern const uint8_t sprite_timer_10[];

#define SPRITE_TIMER_11_WIDTH 60u
#define SPRITE_TIMER_11_HEIGHT 60u
#define SPRITE_TIMER_11_BYTES 1930u
extern const uint8_t sprite_timer_11[];

#define SPRITE_TIMER_2_WIDTH 60u
#define SPRITE_TIMER_2_HEIGHT 60u
#define SPRITE_TIMER_2_BYTES 1927u
extern const uint8_t sprite_timer_2[];

#define SPRITE_TIMER_3_WIDTH 60u
#define SPRITE_TIMER_3_HEIGHT 60u
#define SPRITE_TIMER_3_BYTES 1976u
extern const uint8_t sprite_timer_3[];

#define SPRITE_TIMER_4_WIDTH 60u
#define SPRITE_TIMER_4_HEIGHT 60u
#define SPRITE_TIMER_4_BYTES 2003u
extern const uint8_t sprite_timer_4[];

#define SPRITE_TIMER_5_WIDTH 60u
#define SPRITE_TIMER_5_HEIGHT 60u
#define SPRITE_TIMER_5_BYTES 1934u
extern const uint8_t sprite_timer_5[];

#define SPRITE_TIMER_6_WIDTH 60u
#define SPRITE_TIMER_6_HEIGHT 60u
#define SPRITE_TIMER_6_BYTES 1976u
extern const uint8_t sprite_timer_6[];

#define SPRITE_TIMER_7_WIDTH 60u
#define SPRITE_TIMER_7_HEIGHT 60u
#define SPRITE_TIMER_7_BYTES 1990u
extern const uint8_t sprite_timer_7[];

#define SPRITE_TIMER_8_WIDTH 60u
#define SPRITE_TIMER_8_HEIGHT 60u
#define SPRITE_TIMER_8_BYTES 1945u
extern const uint8_t sprite_timer_8[];

#define SPRITE_TIMER_9_WIDTH 60u
#define SPRITE_TIMER_9_HEIGHT 60u
#define SPRITE_TIMER_9_BYTES 1992u
extern const uint8_t sprite_timer_9[];

#define SPRITE_TRAP_ICON_WIDTH 60u
#define SPRITE_TRAP_ICON_HEIGHT 60u
#define SPRITE_TRAP_ICON_BYTES 3862u
extern const uint8_t sprite_trap_icon[];

#define SPRITE_TVSTEP_UNABLE_WIDTH 135u
#define SPRITE_TVSTEP_UNABLE_HEIGHT 240u
#define SPRITE_TVSTEP_UNABLE_BYTES 13317u
extern const uint8_t sprite_tvstep_unable[];

#define SPRITE_UNSUPPORTED_WIDTH 135u
#define SPRITE_UNSUPPORTED_HEIGHT 240u
#define SPRITE_UNSUPPORTED_BYTES 25010u
extern const uint8_t sprite_unsupported[];

#define SPRITE_VID_ICON_WIDTH 60u
#define SPRITE_VID_ICON_HEIGHT 60u
#define SPRITE_VID_ICON_BYTES 2814u
extern const uint8_t sprite_vid_icon[];

#define SPRITE_WELCOME_WIDTH 135u
#define SPRITE_WELCOME_HEIGHT 240u
#define SPRITE_WELCOME_BYTES 29064u
extern const uint8_t sprite_welcome[];

#define SPRITE_WIFI_CONFIG_WIDTH 135u
#define SPRITE_WIFI_CONFIG_HEIGHT 240u
#define SPRITE_WIFI_CONFIG_BYTES 13047u
extern const uint8_t sprite_wifi_config[];

#define SPRITE_WIFI_ERROR_WIDTH 135u
#define SPRITE_WIFI_ERROR_HEIGHT 240u
#define SPRITE_WIFI_ERROR_BYTES 10781u
extern const uint8_t sprite_wifi_error[];

#define SPRITE_WIFI_REJECT_WIDTH 135u
#define SPRITE_WIFI_REJECT_HEIGHT 240u
#define SPRITE_WIFI_REJECT_BYTES 18363u
extern const uint8_t sprite_wifi_reject[];

#define SPRITE_WIFICFG_FRST_WIDTH 135u
#define SPRITE_WIFICFG_FRST_HEIGHT 240u
#define SPRITE_WIFICFG_FRST_BYTES 9782u
extern const uint8_t sprite_wificfg_frst[];

#define SPRITE_WIFICFG_FRSTDONE_WIDTH 135u
#define SPRITE_WIFICFG_FRSTDONE_HEIGHT 240u
#define SPRITE_WIFICFG_FRSTDONE_BYTES 8980u
extern const uint8_t sprite_wificfg_frstdone[];

#define SPRITE_WIFICFG_HEAD_WIDTH 135u
#define SPRITE_WIFICFG_HEAD_HEIGHT 50u
#define SPRITE_WIFICFG_HEAD_BYTES 6138u
extern const uint8_t sprite_wificfg_head[];

#define SPRITE_WIFICFG_LOGIN_WIDTH 135u
#define SPRITE_WIFICFG_LOGIN_HEIGHT 70u
#define SPRITE_WIFICFG_LOGIN_BYTES 6529u
extern const uint8_t sprite_wificfg_login[];

#define SPRITE_WIFICFG_PROFILESAVE_WIDTH 32u
#define SPRITE_WIFICFG_PROFILESAVE_HEIGHT 32u
#define SPRITE_WIFICFG_PROFILESAVE_BYTES 919u
extern const uint8_t sprite_wificfg_profilesave[];

#define SPRITE_WIFICFG_SELPROFILE_WIDTH 135u
#define SPRITE_WIFICFG_SELPROFILE_HEIGHT 220u
#define SPRITE_WIFICFG_SELPROFILE_BYTES 9528u
extern const uint8_t sprite_wificfg_selprofile[];

#define SPRITE_WIFICFG_URL_WIDTH 135u
#define SPRITE_WIFICFG_URL_HEIGHT 70u
#define SPRITE_WIFICFG_URL_BYTES 6434u
extern const uint8_t sprite_wificfg_url[];

#define SPRITE_WIFIINFO_WIDTH 135u
#define SPRITE_WIFIINFO_HEIGHT 240u
#define SPRITE_WIFIINFO_BYTES 8059u
extern const uint8_t sprite_wifiinfo[];

#define SPRITE_WIFIINFO_HEAD_WIDTH 135u
#define SPRITE_WIFIINFO_HEAD_HEIGHT 50u
#define SPRITE_WIFIINFO_HEAD_BYTES 3068u
extern const uint8_t sprite_wifiinfo_head[];

#define SPRITE_WIFIINFO_LOGIN_WIDTH 135u
#define SPRITE_WIFIINFO_LOGIN_HEIGHT 70u
#define SPRITE_WIFIINFO_LOGIN_BYTES 3433u
extern const uint8_t sprite_wifiinfo_login[];

#define SPRITE_WIFIINFO_URL_WIDTH 135u
#define SPRITE_WIFIINFO_URL_HEIGHT 70u
#define SPRITE_WIFIINFO_URL_BYTES 3331u
extern const uint8_t sprite_wifiinfo_url[];

#define SPRITE_ZOOM_ADJUST_WIDTH 135u
#define SPRITE_ZOOM_ADJUST_HEIGHT 240u
#define SPRITE_ZOOM_ADJUST_BYTES 8042u
extern const uint8_t sprite_zoom_adjust[];
