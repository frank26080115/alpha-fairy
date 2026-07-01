#ifndef _SonyCameraInfraredRemote_H_
#define _SonyCameraInfraredRemote_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 20 bits, repeat 3 times
#define IR_ADDR_SONYCAM    0x1E3A
#define IR_CMD_SHOOT       0x2D
#define IR_CMD_SHOOT_2S    0x37
#define IR_CMD_MOVIE       0x48
#define IR_CMD_ZOOM_TELE1  0x4A
#define IR_CMD_ZOOM_WIDE1  0x4B
#define IR_CMD_ZOOM_TELE2  0x4C
#define IR_CMD_ZOOM_WIDE2  0x4D
#define IR_CMD_MENU        0x38
#define IR_CMD_CENTER_BTN  0x39
#define IR_CMD_UP_BTN      0x3A
#define IR_CMD_DOWN_BTN    0x3B
#define IR_CMD_LEFT_BTN    0x3E
#define IR_CMD_RIGHT_BTN   0x3F
#define IR_CMD_PREVIEW     0x3C
#define IR_CMD_DELETE      0x3D
#define IR_CMD_PLAY        0x47

// the commands below are not divided into address and command, use the raw sending function

// reverse engineered from Sony RMT-845 infrared remote control
#define IR_CMD_RMT845_TCRESET     0x3D7D   // 20 bits, repeat 5 times
#define IR_CMD_RMT845_DATACODE    0x3D33   // 20 bits, repeat 5 times
#define IR_CMD_RMT845_RECORD      0x3D30   // 20 bits, repeat 5 times
#define IR_CMD_RMT845_SLOWLEFT    0x022D22 // 20 bits, repeat 5 times
#define IR_CMD_RMT845_SLOWRIGHT   0x022D23 // 20 bits, repeat 5 times
#define IR_CMD_RMT845_SCANLEFT    0x022D30 // 20 bits, repeat 5 times
#define IR_CMD_RMT845_SCANRIGHT   0x022D31 // 20 bits, repeat 5 times
#define IR_CMD_RMT845_PLAY        0x022D32 // 20 bits, repeat 5 times
#define IR_CMD_RMT845_PAUSE       0x022D39 // 20 bits, repeat 5 times
#define IR_CMD_RMT845_STOP        0x022D38 // 20 bits, repeat 5 times
#define IR_CMD_RMT845_MODE        0x022D1B // 20 bits, repeat 5 times
#define IR_CMD_RMT845_ENTER       0x022D0B // 20 bits, repeat 5 times
#define IR_CMD_RMT845_DISPLAY     0x022D54 // 20 bits, repeat 5 times
#define IR_CMD_RMT845_ARROWLEFT   0x022D7B // 20 bits, repeat 5 times
#define IR_CMD_RMT845_ARROWRIGHT  0x022D7C // 20 bits, repeat 5 times
#define IR_CMD_RMT845_ARROWUP     0x022D79 // 20 bits, repeat 5 times
#define IR_CMD_RMT845_ARROWDOWN   0x022D7A // 20 bits, repeat 5 times
#define IR_CMD_RMT845_ZOOMTELE    0x6C9A   // 15 bits, repeat 5 times or until button release
#define IR_CMD_RMT845_ZOOMWIDE    0x6C9B   // 15 bits, repeat 5 times or until button release

void SonyCamIr_Init(void);
void SonyCamIr_Shoot(void);
void SonyCamIr_Shoot2S(void);
void SonyCamIr_Movie(void);
void SonyCamIr_SendRaw(uint16_t addr, uint8_t cmd);
void SonyCamIr_SendRawX(uint16_t addr, uint8_t cmd, uint8_t xtimes);
void SonyCamIr_SendRawBits(uint32_t data, uint8_t numbits, uint8_t xtimes);

#ifdef __cplusplus
}
#endif

#endif
