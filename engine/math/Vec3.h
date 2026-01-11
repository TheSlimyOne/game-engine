#ifndef GAME_VEC3_H
#define GAME_VEC3_H

#include "Common.h"

struct alignas(16) Vec3 {
    union {
        __m128 mm;
        struct { float x, y, z; };
        float data[4];
    };

    inline Vec3() : mm(_mm_setzero_ps()) {}

    // Scalar constructor (Broadcast)
    inline Vec3(float s) : mm(_mm_set1_ps(s)) {}

    inline Vec3(float _x, float _y, float _z) : mm(_mm_setr_ps(_x, _y, _z, 0.0f)) {}
    inline explicit Vec3(__m128 m) : mm(m) {}

    // --- Assignment Operators ---

    // Default Copy Assignment (v1 = v2)
    inline Vec3& operator=(const Vec3& other) = default;

    // Scalar Assignment (v = 5.0f)
    inline Vec3& operator=(float s) {
        mm = _mm_set1_ps(s);
        return *this;
    }

    // Compound Assignments
    inline Vec3& operator+=(const Vec3& b) { mm = _mm_add_ps(mm, b.mm); return *this; }
    inline Vec3& operator-=(const Vec3& b) { mm = _mm_sub_ps(mm, b.mm); return *this; }
    inline Vec3& operator*=(const Vec3& b) { mm = _mm_mul_ps(mm, b.mm); return *this; } // Component-wise
    inline Vec3& operator*=(float s) { mm = _mm_mul_ps(mm, _mm_set1_ps(s)); return *this; }
    inline Vec3& operator/=(float s) { mm = _mm_div_ps(mm, _mm_set1_ps(s)); return *this; }

    // --- Operators ---
    inline Vec3 operator+(const Vec3& b) const { return Vec3(_mm_add_ps(mm, b.mm)); }
    inline Vec3 operator-(const Vec3& b) const { return Vec3(_mm_sub_ps(mm, b.mm)); }
    inline Vec3 operator*(const Vec3& b) const { return Vec3(_mm_mul_ps(mm, b.mm)); }
    inline Vec3 operator*(float s) const { return Vec3(_mm_mul_ps(mm, _mm_set1_ps(s))); }
    inline Vec3 operator/(float s) const { return Vec3(_mm_div_ps(mm, _mm_set1_ps(s))); }

    // --- Geometric Functions ---
    static inline float dot(const Vec3& a, const Vec3& b) {
        // Mask 0x71: Calc dot for x,y,z (lower 3), store in lowest float
        return _mm_cvtss_f32(_mm_dp_ps(a.mm, b.mm, 0x71));
    }

    static inline Vec3 cross(const Vec3& a, const Vec3& b) {
        // (y, z, x) * (z, x, y) - (z, x, y) * (y, z, x)
        __m128 a_yzx = _mm_shuffle_ps(a.mm, a.mm, _MM_SHUFFLE(3, 0, 2, 1));
        __m128 b_yzx = _mm_shuffle_ps(b.mm, b.mm, _MM_SHUFFLE(3, 0, 2, 1));
        __m128 c = _mm_sub_ps(_mm_mul_ps(a.mm, b_yzx), _mm_mul_ps(a_yzx, b.mm));
        return Vec3(_mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1)));
    }

    inline float norm_sq() const { return dot(*this, *this); }
    inline float norm() const { return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(norm_sq()))); }

    inline Vec3 normalized() const {
        float n = norm_sq();
        if (n < 1e-8f) return Vec3();
        return Vec3(_mm_mul_ps(mm, _mm_rsqrt_ps(_mm_set1_ps(n))));
    }
};

#endif //GAME_VEC3_H