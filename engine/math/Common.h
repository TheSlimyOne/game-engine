#ifndef GAME_COMMON_H
#define GAME_COMMON_H

#include <immintrin.h>
#include <cmath>
#include <algorithm>
#include <cstring>

// Forward Declarations
struct Vec2;
struct Vec3;
struct Vec4;
struct Quat;
struct Mat4;

static const float PI = 3.14159265358979323846f;
static const float DEG2RAD = PI / 180.0f;
static const float RAD2DEG = 180.0f / PI;

inline float radians(float deg) {
    return deg * DEG2RAD;
}

inline float degrees(float rad) {
    return rad * RAD2DEG;
}

#endif //GAME_COMMON_H