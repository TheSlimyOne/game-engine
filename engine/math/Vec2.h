#ifndef GAME_VEC2_H
#define GAME_VEC2_H

#include "Common.h"

struct alignas(16) Vec2 {
    union {
        __m128 mm;
        struct { float x, y; }; // z, w ignored
        float data[4];
    };

    inline Vec2() : mm(_mm_setzero_ps()) {}

    // Scalar constructor (Broadcast)
    inline Vec2(float s) : mm(_mm_set1_ps(s)) {}

    inline Vec2(float _x, float _y) : mm(_mm_setr_ps(_x, _y, 0.0f, 0.0f)) {}
    inline explicit Vec2(__m128 m) : mm(m) {}

    // --- Assignment Operators ---

    // Default Copy Assignment (v1 = v2)
    inline Vec2& operator=(const Vec2& other) = default;

    // Scalar Assignment (v = 5.0f)
    inline Vec2& operator=(float s) {
        mm = _mm_set1_ps(s);
        return *this;
    }

    // Compound Assignments
    inline Vec2& operator+=(const Vec2& b) { mm = _mm_add_ps(mm, b.mm); return *this; }
    inline Vec2& operator-=(const Vec2& b) { mm = _mm_sub_ps(mm, b.mm); return *this; }
    inline Vec2& operator*=(const Vec2& b) { mm = _mm_mul_ps(mm, b.mm); return *this; } // Component-wise
    inline Vec2& operator*=(float s) { mm = _mm_mul_ps(mm, _mm_set1_ps(s)); return *this; }
    inline Vec2& operator/=(float s) { mm = _mm_div_ps(mm, _mm_set1_ps(s)); return *this; }

    // --- Operators ---
    inline Vec2 operator+(const Vec2& b) const { return Vec2(_mm_add_ps(mm, b.mm)); }
    inline Vec2 operator-(const Vec2& b) const { return Vec2(_mm_sub_ps(mm, b.mm)); }
    inline Vec2 operator*(const Vec2& b) const { return Vec2(_mm_mul_ps(mm, b.mm)); }
    inline Vec2 operator*(float s) const { return Vec2(_mm_mul_ps(mm, _mm_set1_ps(s))); }
    inline Vec2 operator/(float s) const { return Vec2(_mm_div_ps(mm, _mm_set1_ps(s))); }

    // --- Geometric ---
    static inline float dot(const Vec2& a, const Vec2& b) {
        // 0x31: Mask 0011 (calc x,y), store in the lowest float
        return _mm_cvtss_f32(_mm_dp_ps(a.mm, b.mm, 0x31));
    }

    inline float norm_sq() const { return dot(*this, *this); }
    inline float norm() const { return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(norm_sq()))); }

    inline Vec2 normalized() const {
        float n = norm_sq();
        if (n < 1e-8f) return Vec2();
        return Vec2(_mm_mul_ps(mm, _mm_rsqrt_ps(_mm_set1_ps(n))));
    }
};

#endif //GAME_VEC2_H