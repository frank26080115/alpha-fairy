#ifndef _SonyCameraInfraredRemote_H_
#define _SonyCameraInfraredRemote_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
