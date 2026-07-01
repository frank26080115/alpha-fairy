#include "MahonyAHRS.h"

#include <Arduino.h>
#include <math.h>

#define SAMPLE_FREQ 25.0f
#define TWO_KP_DEF  (2.0f * 1.0f)
#define TWO_KI_DEF  (2.0f * 0.0f)

volatile float twoKp = TWO_KP_DEF;
volatile float twoKi = TWO_KI_DEF;

static volatile float q0 = 1.0f;
static volatile float q1 = 0.0f;
static volatile float q2 = 0.0f;
static volatile float q3 = 0.0f;
static volatile float integralFBx = 0.0f;
static volatile float integralFBy = 0.0f;
static volatile float integralFBz = 0.0f;

void MahonyAHRSupdateIMU(float gx, float gy, float gz, float ax, float ay,
                         float az, float *pitch, float *roll, float *yaw)
{
    float recipNorm;
    float halfvx, halfvy, halfvz;
    float halfex, halfey, halfez;
    float qa, qb, qc;

    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
        recipNorm = invSqrt(ax * ax + ay * ay + az * az);
        ax *= recipNorm;
        ay *= recipNorm;
        az *= recipNorm;

        halfvx = q1 * q3 - q0 * q2;
        halfvy = q0 * q1 + q2 * q3;
        halfvz = q0 * q0 - 0.5f + q3 * q3;

        halfex = ay * halfvz - az * halfvy;
        halfey = az * halfvx - ax * halfvz;
        halfez = ax * halfvy - ay * halfvx;

        if (twoKi > 0.0f) {
            integralFBx += twoKi * halfex * (1.0f / SAMPLE_FREQ);
            integralFBy += twoKi * halfey * (1.0f / SAMPLE_FREQ);
            integralFBz += twoKi * halfez * (1.0f / SAMPLE_FREQ);
            gx += integralFBx;
            gy += integralFBy;
            gz += integralFBz;
        }
        else {
            integralFBx = 0.0f;
            integralFBy = 0.0f;
            integralFBz = 0.0f;
        }

        gx += twoKp * halfex;
        gy += twoKp * halfey;
        gz += twoKp * halfez;
    }

    gx *= 0.5f * (1.0f / SAMPLE_FREQ);
    gy *= 0.5f * (1.0f / SAMPLE_FREQ);
    gz *= 0.5f * (1.0f / SAMPLE_FREQ);
    qa = q0;
    qb = q1;
    qc = q2;
    q0 += -qb * gx - qc * gy - q3 * gz;
    q1 += qa * gx + qc * gz - q3 * gy;
    q2 += qa * gy - qb * gz + q3 * gx;
    q3 += qa * gz + qb * gy - qc * gx;

    recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;

    *pitch = asin(-2 * q1 * q3 + 2 * q0 * q2);
    *roll  = atan2(2 * q2 * q3 + 2 * q0 * q1,
                   -2 * q1 * q1 - 2 * q2 * q2 + 1);
    *yaw   = atan2(2 * (q1 * q2 + q0 * q3),
                   q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3);

    *pitch *= RAD_TO_DEG;
    *yaw *= RAD_TO_DEG;
    *yaw -= 8.5f;
    *roll *= RAD_TO_DEG;
}

float invSqrt(float x)
{
    float halfx = 0.5f * x;
    float y = x;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
    long i = *(long *)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float *)&i;
#pragma GCC diagnostic pop
    y = y * (1.5f - (halfx * y * y));
    return y;
}
