#pragma once

#include <cmath>

#define PI 3.14159f

#define ToRadian(x) ((x) * 3.14159f /180.0f)
#define ToDegree(x) ((x) * 180.0f / 3.14159f)

inline float LinearMap(float x, float fromMin, float fromMax, float toMin, float toMax)
{
    return toMin + ((x - fromMin) / (fromMax - fromMin)) * (toMax - toMin);
}

inline int mod_floor(int a, int n) {
    return ((a % n) + n) % n;
}

inline int mod_floor(int a, size_t n) {
    return mod_floor(a, (int)n);
}

template<typename T>
inline T clamp(T val, T min, T max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

double generateGaussian(double mean, double stdDev);