#ifndef _ALPHAFAIRY_MAHONYAHRS_H_
#define _ALPHAFAIRY_MAHONYAHRS_H_

extern volatile float twoKp;
extern volatile float twoKi;

void MahonyAHRSupdateIMU(float gx, float gy, float gz, float ax, float ay,
                         float az, float *pitch, float *roll, float *yaw);
float invSqrt(float x);

#endif
