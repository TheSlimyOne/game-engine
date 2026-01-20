// Mat3.h
#ifndef GAME_MAT3_H
#define GAME_MAT3_H

#include "Common.h"
#include "Vec3.h"
#include "Quat.h"

#include <cmath>
#include <cstring>

// Column-major 3x3, stored as 3x __m128 columns with W padding (always 0).
// Layout in memory (data[12]):
//   col0: [m00, m01, m02, 0]
//   col1: [m10, m11, m12, 0]
//   col2: [m20, m21, m22, 0]
struct alignas(16) Mat3 {
    union {
        __m128 cols[3];
        float  data[12];
    };

    // ------------------------------------------------------------
    // Constructors
    // ------------------------------------------------------------
    inline Mat3() {
        cols[0] = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
        cols[1] = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
        cols[2] = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
    }

    // Expects 9 floats in column-major order: [col0.xyz, col1.xyz, col2.xyz]
    inline Mat3(const float* ptr9) {
        cols[0] = _mm_setr_ps(ptr9[0], ptr9[1], ptr9[2], 0.0f);
        cols[1] = _mm_setr_ps(ptr9[3], ptr9[4], ptr9[5], 0.0f);
        cols[2] = _mm_setr_ps(ptr9[6], ptr9[7], ptr9[8], 0.0f);
    }

    inline explicit Mat3(__m128 c0, __m128 c1, __m128 c2) {
        cols[0] = _mm_and_ps(c0, _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0)));
        cols[1] = _mm_and_ps(c1, _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0)));
        cols[2] = _mm_and_ps(c2, _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0)));
    }

    // Rotation + Scale (no translation)
    inline Mat3(const Quat& r, const Vec3& s) {
        float x = r.x, y = r.y, z = r.z, w = r.w;
        float x2 = x + x, y2 = y + y, z2 = z + z;
        float xx = x * x2, xy = x * y2, xz = x * z2;
        float yy = y * y2, yz = y * z2, zz = z * z2;
        float wx = w * x2, wy = w * y2, wz = w * z2;

        cols[0] = _mm_mul_ps(_mm_setr_ps(1.0f - (yy + zz), xy + wz,       xz - wy,       0.0f), _mm_set1_ps(s.x));
        cols[1] = _mm_mul_ps(_mm_setr_ps(xy - wz,         1.0f - (xx + zz), yz + wx,       0.0f), _mm_set1_ps(s.y));
        cols[2] = _mm_mul_ps(_mm_setr_ps(xz + wy,         yz - wx,         1.0f - (xx + yy),0.0f), _mm_set1_ps(s.z));
    }

    inline Mat3(const Quat& r) : Mat3(r, Vec3(1, 1, 1)) {}

    // ------------------------------------------------------------
    // GLM-like pointer access
    // NOTE: ptr() returns 12 floats (padded W per column). This is std140-friendly and SIMD-friendly.
    // If you need tightly packed 9 floats, call store_packed9().
    // ------------------------------------------------------------
    inline const float* ptr() const { return data; }
    inline float* ptr() { return data; }

    inline void store_packed9(float out9[9]) const {
        // Column-major packed 3x3
        out9[0] = data[0];  out9[1] = data[1];  out9[2] = data[2];
        out9[3] = data[4];  out9[4] = data[5];  out9[5] = data[6];
        out9[6] = data[8];  out9[7] = data[9];  out9[8] = data[10];
    }

    // ------------------------------------------------------------
    // Direction vectors (assuming basis in columns)
    // ------------------------------------------------------------
    inline Vec3 right() const   { return Vec3(cols[0]).normalized(); }
    inline Vec3 up() const      { return Vec3(cols[1]).normalized(); }
    inline Vec3 forward() const { return (Vec3(cols[2]) * -1.0f).normalized(); } // -Z forward (matches your Mat4)

    // ------------------------------------------------------------
    // Core operations
    // ------------------------------------------------------------
    static inline Mat3 identity() {
        Mat3 m;
        return m;
    }

    static inline Mat3 mul(const Mat3& a, const Mat3& b) {
        Mat3 res;
        for (int i = 0; i < 3; ++i) {
            __m128 b_col = b.cols[i];

            __m128 x = _mm_shuffle_ps(b_col, b_col, _MM_SHUFFLE(0, 0, 0, 0));
            __m128 y = _mm_shuffle_ps(b_col, b_col, _MM_SHUFFLE(1, 1, 1, 1));
            __m128 z = _mm_shuffle_ps(b_col, b_col, _MM_SHUFFLE(2, 2, 2, 2));

            __m128 sum = _mm_mul_ps(a.cols[0], x);
            sum = _mm_add_ps(sum, _mm_mul_ps(a.cols[1], y));
            sum = _mm_add_ps(sum, _mm_mul_ps(a.cols[2], z));

            // Ensure W stays 0
            sum = _mm_and_ps(sum, _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0)));
            res.cols[i] = sum;
        }
        return res;
    }

    inline Mat3 operator*(const Mat3& b) const { return mul(*this, b); }

    // Mat3 * scalar
    inline Mat3 operator*(float s) const {
        __m128 ss = _mm_set1_ps(s);
        Mat3 r;
        r.cols[0] = _mm_mul_ps(cols[0], ss);
        r.cols[1] = _mm_mul_ps(cols[1], ss);
        r.cols[2] = _mm_mul_ps(cols[2], ss);
        return r;
    }

    // Mat3 * Vec3 (column-major): M*v = col0*v.x + col1*v.y + col2*v.z
    inline Vec3 operator*(const Vec3& v) const {
        __m128 vx = _mm_set1_ps(v.x);
        __m128 vy = _mm_set1_ps(v.y);
        __m128 vz = _mm_set1_ps(v.z);

        __m128 sum = _mm_mul_ps(cols[0], vx);
        sum = _mm_add_ps(sum, _mm_mul_ps(cols[1], vy));
        sum = _mm_add_ps(sum, _mm_mul_ps(cols[2], vz));

        // W is already 0, but keep it clean
        sum = _mm_and_ps(sum, _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0)));
        return Vec3(sum);
    }

    // Transpose (3x3)
    inline Mat3 transpose() const {
        Mat3 res;

        __m128 c0 = cols[0];
        __m128 c1 = cols[1];
        __m128 c2 = cols[2];

        // tmp0 = [c0.x, c1.x, c0.y, c1.y]
        __m128 tmp0 = _mm_unpacklo_ps(c0, c1);
        // tmp1 = [c0.z, c1.z, c0.w, c1.w]
        __m128 tmp1 = _mm_unpackhi_ps(c0, c1);

        // res.col0 = [c0.x, c1.x, c2.x, c2.w(=0)]
        res.cols[0] = _mm_shuffle_ps(tmp0, c2, 0xC4);
        // res.col1 = [c0.y, c1.y, c2.y, c2.w(=0)]
        res.cols[1] = _mm_shuffle_ps(tmp0, c2, 0xDE);
        // res.col2 = [c0.z, c1.z, c2.z, c2.w(=0)]
        res.cols[2] = _mm_shuffle_ps(tmp1, c2, 0xE4);

        // Force W = 0
        __m128i mask = _mm_setr_epi32(-1, -1, -1, 0);
        res.cols[0] = _mm_and_ps(res.cols[0], _mm_castsi128_ps(mask));
        res.cols[1] = _mm_and_ps(res.cols[1], _mm_castsi128_ps(mask));
        res.cols[2] = _mm_and_ps(res.cols[2], _mm_castsi128_ps(mask));

        return res;
    }

    // Determinant via det = dot(c0, cross(c1, c2))
    inline float determinant() const {
        __m128 c0 = cols[0];
        __m128 c1 = cols[1];
        __m128 c2 = cols[2];

        __m128 cr = cross3(c1, c2);
        __m128 d  = _mm_dp_ps(c0, cr, 0x71); // xyz dot -> x
        return _mm_cvtss_f32(d);
    }

    // Inverse optimized for 3x3 using cross/dot.
    // Returns Identity if determinant is too close to zero.
    inline Mat3 inverse() const {
        __m128 c0 = cols[0];
        __m128 c1 = cols[1];
        __m128 c2 = cols[2];

        // Rows of inverse (dual basis)
        __m128 r0 = cross3(c1, c2);
        __m128 r1 = cross3(c2, c0);
        __m128 r2 = cross3(c0, c1);

        float det = _mm_cvtss_f32(_mm_dp_ps(c0, r0, 0x71));
        if (std::abs(det) < 1e-8f) return identity();

        __m128 invDet = _mm_div_ps(_mm_set1_ps(1.0f), _mm_set1_ps(det));

        // Inverse has rows [r0; r1; r2] / det.
        // Convert rows -> column-major by transposing the matrix that has columns (r0,r1,r2).
        Mat3 adjCols(r0, r1, r2);
        Mat3 inv = adjCols.transpose();

        inv.cols[0] = _mm_mul_ps(inv.cols[0], invDet);
        inv.cols[1] = _mm_mul_ps(inv.cols[1], invDet);
        inv.cols[2] = _mm_mul_ps(inv.cols[2], invDet);

        // Force W=0
        __m128i mask = _mm_setr_epi32(-1, -1, -1, 0);
        inv.cols[0] = _mm_and_ps(inv.cols[0], _mm_castsi128_ps(mask));
        inv.cols[1] = _mm_and_ps(inv.cols[1], _mm_castsi128_ps(mask));
        inv.cols[2] = _mm_and_ps(inv.cols[2], _mm_castsi128_ps(mask));

        return inv;
    }

    // Fast inverse for orthonormal rotation matrices (no scale/shear): inverse == transpose
    inline Mat3 inverse_orthonormal() const { return transpose(); }

    // ------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------
    static inline Mat3 scale(const Vec3& v) {
        Mat3 m;
        m.cols[0] = _mm_setr_ps(v.x, 0.0f, 0.0f, 0.0f);
        m.cols[1] = _mm_setr_ps(0.0f, v.y, 0.0f, 0.0f);
        m.cols[2] = _mm_setr_ps(0.0f, 0.0f, v.z, 0.0f);
        return m;
    }

    static inline Mat3 rotate(const Quat& q) {
        return Mat3(q);
    }

private:
    // 3D cross product on xyz, keeps w=0
    static inline __m128 cross3(__m128 a, __m128 b) {
        // a = [x y z w], b = [x y z w]
        __m128 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1)); // [y z x w]
        __m128 b_zxy = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2)); // [z x y w]
        __m128 a_zxy = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2)); // [z x y w]
        __m128 b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1)); // [y z x w]

        __m128 c = _mm_sub_ps(_mm_mul_ps(a_yzx, b_zxy), _mm_mul_ps(a_zxy, b_yzx));
        // Back to [x y z w]
        c = _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));

        // Force w=0
        return _mm_and_ps(c, _mm_castsi128_ps(_mm_setr_epi32(-1, -1, -1, 0)));
    }
};

#endif // GAME_MAT3_H
