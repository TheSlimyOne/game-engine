#ifndef GAME_VEC4_H
#define GAME_VEC4_H

#include "Common.h"
#include "Vec3.h"

struct alignas(16) Vec4 {
    union {
        __m128 mm;
        struct { float x, y, z, w; };
        float data[4];
    };

    inline Vec4() : mm(_mm_setzero_ps()) {}

    // --- NEW: Scalar constructor (Broadcast) ---
    // Sets x, y, z, w to s
    inline Vec4(float s) : mm(_mm_set1_ps(s)) {}

    inline Vec4(float _x, float _y, float _z, float _w) : mm(_mm_setr_ps(_x, _y, _z, _w)) {}

    // Construct from Vec3 + w
    inline Vec4(const Vec3& v, float _w) {
        alignas(16) float tmp[4];
        _mm_store_ps(tmp, v.mm);
        tmp[3] = _w;
        mm = _mm_load_ps(tmp);
    }

    inline explicit Vec4(__m128 m) : mm(m) {}

    // --- Assignment Operators ---

    // Default Copy Assignment
    inline Vec4& operator=(const Vec4& other) = default;

    // Scalar Assignment (v = 1.0f)
    inline Vec4& operator=(float s) {
        mm = _mm_set1_ps(s);
        return *this;
    }

    // Compound Assignments
    inline Vec4& operator+=(const Vec4& b) { mm = _mm_add_ps(mm, b.mm); return *this; }
    inline Vec4& operator-=(const Vec4& b) { mm = _mm_sub_ps(mm, b.mm); return *this; }
    inline Vec4& operator*=(const Vec4& b) { mm = _mm_mul_ps(mm, b.mm); return *this; } // Component-wise
    inline Vec4& operator*=(float s) { mm = _mm_mul_ps(mm, _mm_set1_ps(s)); return *this; }
    inline Vec4& operator/=(float s) { mm = _mm_div_ps(mm, _mm_set1_ps(s)); return *this; }

    // --- Operators ---
    inline Vec4 operator+(const Vec4& b) const { return Vec4(_mm_add_ps(mm, b.mm)); }
    inline Vec4 operator-(const Vec4& b) const { return Vec4(_mm_sub_ps(mm, b.mm)); }
    inline Vec4 operator*(const Vec4& b) const { return Vec4(_mm_mul_ps(mm, b.mm)); }
    inline Vec4 operator*(float s) const { return Vec4(_mm_mul_ps(mm, _mm_set1_ps(s))); }
    inline Vec4 operator/(float s) const { return Vec4(_mm_div_ps(mm, _mm_set1_ps(s))); }

    // --- Geometric ---
    static inline float dot(const Vec4& a, const Vec4& b) {
        // 0xF1: Mask 1111 (calc x,y,z,w), store in lowest float
        return _mm_cvtss_f32(_mm_dp_ps(a.mm, b.mm, 0xF1));
    }

    inline float norm_sq() const { return dot(*this, *this); }
    inline float norm() const { return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(norm_sq()))); }

    inline Vec4 normalized() const {
        float n = norm_sq();
        if (n < 1e-8f) return Vec4();
        return Vec4(_mm_mul_ps(mm, _mm_rsqrt_ps(_mm_set1_ps(n))));
    }
};

#endif //GAME_VEC4_H