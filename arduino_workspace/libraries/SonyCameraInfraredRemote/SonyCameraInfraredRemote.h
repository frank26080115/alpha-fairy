#ifndef _SonyCameraInfraredRemote_H_
#define _SonyCameraInfraredRemote_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void SonyCamIr_Init(void);
void SonyCamIr_Shoot(void);
void SonyCamIr_Shoot2S(void);
void SonyCamIr_Movie(void);
void SonyCamIr_SendRaw(uint16_t addr, uint8_t cmd);

#ifdef __cplusplus
}
#endif

#endif
