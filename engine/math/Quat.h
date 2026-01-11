#ifndef GAME_QUAT_H
#define GAME_QUAT_H

#include "Common.h"
#include "Vec3.h"

struct alignas(16) Quat {
    union {
        __m128 mm;
        struct { float x, y, z, w; };
        float data[4];
    };

    inline Quat() : mm(_mm_setr_ps(0.f, 0.f, 0.f, 1.f)) {}
    inline Quat(float _x, float _y, float _z, float _w) : mm(_mm_setr_ps(_x, _y, _z, _w)) {}
    inline explicit Quat(__m128 m) : mm(m) {}

    static inline Quat identity() { return Quat(0, 0, 0, 1); }

    static inline Quat angle_axis(float angle, const Vec3& axis) {
        float s = std::sin(angle * 0.5f);
        float c = std::cos(angle * 0.5f);
        Vec3 v = axis.normalized() * s;
        return Quat(v.x, v.y, v.z, c);
    }

    // Constructs a rotation from Euler angles (x=Pitch, y=Yaw, z=Roll).
    // Applies rotations in the order: X, then Y, then Z (q = qz * qy * qx)
    static inline Quat from_euler(const Vec3& rotation) {
        float cx = std::cos(rotation.x * 0.5f);
        float sx = std::sin(rotation.x * 0.5f);
        float cy = std::cos(rotation.y * 0.5f);
        float sy = std::sin(rotation.y * 0.5f);
        float cz = std::cos(rotation.z * 0.5f);
        float sz = std::sin(rotation.z * 0.5f);

        return Quat(
            sx * cy * cz - cx * sy * sz, // X
            cx * sy * cz + sx * cy * sz, // Y
            cx * cy * sz - sx * sy * cz, // Z
            cx * cy * cz + sx * sy * sz  // W
        );
    }

    // --- Operators ---
    inline Quat operator+(const Quat& b) const { return Quat(_mm_add_ps(mm, b.mm)); }
    inline Quat operator-(const Quat& b) const { return Quat(_mm_sub_ps(mm, b.mm)); }
    inline Quat operator*(float s) const { return Quat(_mm_mul_ps(mm, _mm_set1_ps(s))); }

    inline Quat operator*(const Quat& q) const {
        __m128 w_w_w_w = _mm_shuffle_ps(mm, mm, _MM_SHUFFLE(3, 3, 3, 3));
        __m128 res = _mm_mul_ps(w_w_w_w, q.mm);

        __m128 x_x_x_x = _mm_shuffle_ps(mm, mm, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 q_swiz1 = _mm_shuffle_ps(q.mm, q.mm, _MM_SHUFFLE(0, 1, 2, 3));
        res = _mm_add_ps(res, _mm_mul_ps(x_x_x_x, _mm_xor_ps(q_swiz1, _mm_setr_ps(0.f, -0.f, 0.f, -0.f))));

        __m128 y_y_y_y = _mm_shuffle_ps(mm, mm, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 q_swiz2 = _mm_shuffle_ps(q.mm, q.mm, _MM_SHUFFLE(1, 0, 3, 2));
        res = _mm_add_ps(res, _mm_mul_ps(y_y_y_y, _mm_xor_ps(q_swiz2, _mm_setr_ps(0.f, 0.f, -0.f, -0.f))));

        __m128 z_z_z_z = _mm_shuffle_ps(mm, mm, _MM_SHUFFLE(2, 2, 2, 2));
        __m128 q_swiz3 = _mm_shuffle_ps(q.mm, q.mm, _MM_SHUFFLE(2, 3, 0, 1));
        res = _mm_add_ps(res, _mm_mul_ps(z_z_z_z, _mm_xor_ps(q_swiz3, _mm_setr_ps(-0.f, 0.f, 0.f, -0.f))));

        return Quat(res);
    }

    // --- Geometric ---
    static inline float dot(const Quat& a, const Quat& b) {
        return _mm_cvtss_f32(_mm_dp_ps(a.mm, b.mm, 0xF1));
    }
    inline float norm_sq() const { return dot(*this, *this); }
    inline float norm() const { return std::sqrt(norm_sq()); }

    inline Quat normalize() const {
        float n = norm_sq();
        if (n < 1e-8f) return identity();
        return Quat(_mm_mul_ps(mm, _mm_rsqrt_ps(_mm_set1_ps(n))));
    }

    inline Quat conjugate() const {
        return Quat(_mm_mul_ps(mm, _mm_setr_ps(-1.f, -1.f, -1.f, 1.f)));
    }

    inline Quat inverse() const {
        float n = norm_sq();
        if (n < 1e-8f) return identity();
        return conjugate() * (1.0f / n);
    }
};

#endif //GAME_QUAT_H