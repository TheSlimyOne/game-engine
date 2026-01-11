#ifndef GAME_MAT4_H
#define GAME_MAT4_H

#include "Common.h"
#include "Vec3.h"
#include "Quat.h"

struct alignas(16) Mat4 {
    union {
        __m128 cols[4];
        float data[16];
    };

    inline Mat4() {
        cols[0] = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
        cols[1] = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
        cols[2] = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
        cols[3] = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
    }
    inline Mat4(const float* ptr) { memcpy(data, ptr, 16 * sizeof(float)); }
    inline explicit Mat4(__m128 c0, __m128 c1, __m128 c2, __m128 c3) {
        cols[0] = c0; cols[1] = c1; cols[2] = c2; cols[3] = c3;
    }

    // --- TRS Constructor ---
    inline Mat4(const Vec3& t, const Quat& r, const Vec3& s) {
        float x = r.x, y = r.y, z = r.z, w = r.w;
        float x2 = x + x, y2 = y + y, z2 = z + z;
        float xx = x * x2, xy = x * y2, xz = x * z2;
        float yy = y * y2, yz = y * z2, zz = z * z2;
        float wx = w * x2, wy = w * y2, wz = w * z2;

        cols[0] = _mm_mul_ps(_mm_setr_ps(1.0f - (yy + zz), xy + wz, xz - wy, 0.0f), _mm_set1_ps(s.x));
        cols[1] = _mm_mul_ps(_mm_setr_ps(xy - wz, 1.0f - (xx + zz), yz + wx, 0.0f), _mm_set1_ps(s.y));
        cols[2] = _mm_mul_ps(_mm_setr_ps(xz + wy, yz - wx, 1.0f - (xx + yy), 0.0f), _mm_set1_ps(s.z));
        cols[3] = _mm_setr_ps(t.x, t.y, t.z, 1.0f);
    }

    // --- GLM Compatibility: value_ptr ---
    inline const float* ptr() const { return data; }
    inline float* ptr() { return data; }

    // --- Direction Vectors ---
    inline Vec3 right() const { return Vec3(cols[0]).normalized(); }
    inline Vec3 up() const { return Vec3(cols[1]).normalized(); }
    inline Vec3 forward() const { return (Vec3(cols[2]) * -1.0f).normalized(); } // -Z forward

    // --- Core Operations ---
    static inline Mat4 identity() {
        Mat4 m; // This calls the constructor above, which sets it to Identity.
        return m;
    }

    static inline Mat4 mul(const Mat4& a, const Mat4& b) {
        Mat4 res;
        for (int i = 0; i < 4; ++i) {
            __m128 b_col = b.cols[i];
            __m128 x = _mm_shuffle_ps(b_col, b_col, _MM_SHUFFLE(0, 0, 0, 0));
            __m128 y = _mm_shuffle_ps(b_col, b_col, _MM_SHUFFLE(1, 1, 1, 1));
            __m128 z = _mm_shuffle_ps(b_col, b_col, _MM_SHUFFLE(2, 2, 2, 2));
            __m128 w = _mm_shuffle_ps(b_col, b_col, _MM_SHUFFLE(3, 3, 3, 3));
            __m128 sum = _mm_mul_ps(a.cols[0], x);
            sum = _mm_add_ps(sum, _mm_mul_ps(a.cols[1], y));
            sum = _mm_add_ps(sum, _mm_mul_ps(a.cols[2], z));
            sum = _mm_add_ps(sum, _mm_mul_ps(a.cols[3], w));
            res.cols[i] = sum;
        }
        return res;
    }

    inline Mat4 operator*(const Mat4& b) const { return mul(*this, b); }


    // --- Transpose Operation ---
    // Swaps Rows and Columns. Essential for fixing the "Massive Cube" issue
    // when sending instance data to the GPU.
    inline Mat4 transpose() const {
        Mat4 res;

        // Load columns
        __m128 r0 = cols[0];
        __m128 r1 = cols[1];
        __m128 r2 = cols[2];
        __m128 r3 = cols[3];

        // Merge inputs to temporary rows
        // tmp0 = [r0x, r1x, r0y, r1y]
        // tmp1 = [r0z, r1z, r0w, r1w]
        __m128 tmp0 = _mm_unpacklo_ps(r0, r1);
        __m128 tmp1 = _mm_unpackhi_ps(r0, r1);

        // tmp2 = [r2x, r3x, r2y, r3y]
        // tmp3 = [r2z, r3z, r2w, r3w]
        __m128 tmp2 = _mm_unpacklo_ps(r2, r3);
        __m128 tmp3 = _mm_unpackhi_ps(r2, r3);

        // Shuffle to final columns
        // res[0] = [r0x, r1x, r2x, r3x] (Old Row 0 is now Col 0)
        res.cols[0] = _mm_movelh_ps(tmp0, tmp2);

        // res[1] = [r0y, r1y, r2y, r3y] (Old Row 1 is now Col 1)
        res.cols[1] = _mm_movehl_ps(tmp2, tmp0);

        // res[2] = [r0z, r1z, r2z, r3z] (Old Row 2 is now Col 2)
        res.cols[2] = _mm_movelh_ps(tmp1, tmp3);

        // res[3] = [r0w, r1w, r2w, r3w] (Old Row 3 is now Col 3)
        res.cols[3] = _mm_movehl_ps(tmp3, tmp1);

        return res;
    }


    // --- Fast View Matrix Calculation ---
    // Calculates (M^-1) assuming M is a rigid body transform (Rotation + Translation, no scale).
    // This is significantly faster than the general purpose inverse().
    static inline Mat4 inverse_affine(const Mat4& in) {
        // 1. Transpose the Rotation part (Top-Left 3x3)
        // We use standard shuffles to transpose the 4x4, but we'll mask out the bottom row later.
        __m128 t0 = _mm_shuffle_ps(in.cols[0], in.cols[1], 0x44); // 00, 01, 10, 11
        __m128 t1 = _mm_shuffle_ps(in.cols[2], in.cols[3], 0x44); // 20, 21, 30, 31 -- Note: 3x are garbage for rot, but ok
        __m128 t2 = _mm_shuffle_ps(in.cols[0], in.cols[1], 0xEE); // 02, 03, 12, 13
        __m128 t3 = _mm_shuffle_ps(in.cols[2], in.cols[3], 0xEE); // 22, 23, 32, 33

        // Construct the new columns (which are the rows of the original rotation)
        // We force the W component to 0 for these basis vectors
        __m128 c0 = _mm_shuffle_ps(t0, t1, 0x88); // 00, 10, 20, 30 -> Masked later to: R0x, R1x, R2x, 0
        __m128 c1 = _mm_shuffle_ps(t0, t1, 0xDD); // 01, 11, 21, 31 -> Masked later to: R0y, R1y, R2y, 0
        __m128 c2 = _mm_shuffle_ps(t2, t3, 0x88); // 02, 12, 22, 32 -> Masked later to: R0z, R1z, R2z, 0

        // Zero out W for the 3x3 rotation columns
        // cast to int, AND with mask, cast back
        __m128i mask = _mm_setr_epi32(-1, -1, -1, 0);
        c0 = _mm_and_ps(c0, _mm_castsi128_ps(mask));
        c1 = _mm_and_ps(c1, _mm_castsi128_ps(mask));
        c2 = _mm_and_ps(c2, _mm_castsi128_ps(mask));

        // 2. Calculate new Translation Column: T' = -(R_transpose * T)
        // This equates to: dot(BasisX, Pos), dot(BasisY, Pos), dot(BasisZ, Pos) negated.
        __m128 pos = in.cols[3];

        // DPPS 0x71: Dot product of xyz, store in x
        // DPPS 0x72: Dot product of xyz, store in y
        // DPPS 0x74: Dot product of xyz, store in z
        // DPPS 0x7F: Dot product of xyz, store in all (not efficient here)

        // Dot products (using original columns against position)
        // T'.x = -dot(OriginalCol0, Pos)
        // T'.y = -dot(OriginalCol1, Pos)
        // T'.z = -dot(OriginalCol2, Pos)
        __m128 d0 = _mm_dp_ps(in.cols[0], pos, 0x71); // Store in X
        __m128 d1 = _mm_dp_ps(in.cols[1], pos, 0x72); // Store in Y
        __m128 d2 = _mm_dp_ps(in.cols[2], pos, 0x74); // Store in Z

        __m128 newPos = _mm_or_ps(_mm_or_ps(d0, d1), d2); // Merge x, y, z
        newPos = _mm_xor_ps(newPos, _mm_set1_ps(-0.0f)); // Negate

        // Set W to 1.0f
        newPos = _mm_add_ps(newPos, _mm_setr_ps(0.f, 0.f, 0.f, 1.f));

        return Mat4(c0, c1, c2, newPos);
    }

    // Optimized 4x4 Matrix Inverse using SIMD cofactor expansion.
    // Returns Identity if determinant is too close to zero (Singular).
    inline Mat4 inverse() const {
        // Transpose the matrix first to make shuffling easier for cofactor calc
        __m128 tmp0 = _mm_shuffle_ps(cols[0], cols[1], 0x44);
        __m128 tmp2 = _mm_shuffle_ps(cols[0], cols[1], 0xEE);
        __m128 tmp1 = _mm_shuffle_ps(cols[2], cols[3], 0x44);
        __m128 tmp3 = _mm_shuffle_ps(cols[2], cols[3], 0xEE);

        __m128 row0 = _mm_shuffle_ps(tmp0, tmp1, 0x88);
        __m128 row1 = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
        __m128 row2 = _mm_shuffle_ps(tmp2, tmp3, 0x88);
        __m128 row3 = _mm_shuffle_ps(tmp2, tmp3, 0xDD);

        // Calculate 2x2 determinants
        __m128 V00 = _mm_shuffle_ps(row2, row2, _MM_SHUFFLE(1, 1, 0, 0));
        __m128 V10 = _mm_shuffle_ps(row3, row3, _MM_SHUFFLE(3, 2, 3, 2));
        __m128 V01 = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(1, 1, 0, 0));
        __m128 V11 = _mm_shuffle_ps(row1, row1, _MM_SHUFFLE(3, 2, 3, 2));
        __m128 V02 = _mm_shuffle_ps(row2, row0, _MM_SHUFFLE(2, 0, 2, 0));
        __m128 V12 = _mm_shuffle_ps(row3, row1, _MM_SHUFFLE(3, 1, 3, 1));

        __m128 D0 = _mm_mul_ps(V00, V10);
        __m128 D1 = _mm_mul_ps(V01, V11);
        __m128 D2 = _mm_mul_ps(V02, V12);

        V00 = _mm_shuffle_ps(row2, row2, _MM_SHUFFLE(3, 2, 3, 2));
        V10 = _mm_shuffle_ps(row3, row3, _MM_SHUFFLE(1, 1, 0, 0));
        V01 = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(3, 2, 3, 2));
        V11 = _mm_shuffle_ps(row1, row1, _MM_SHUFFLE(1, 1, 0, 0));
        V02 = _mm_shuffle_ps(row2, row0, _MM_SHUFFLE(3, 1, 3, 1));
        V12 = _mm_shuffle_ps(row3, row1, _MM_SHUFFLE(2, 0, 2, 0));

        D0 = _mm_sub_ps(D0, _mm_mul_ps(V00, V10));
        D1 = _mm_sub_ps(D1, _mm_mul_ps(V01, V11));
        D2 = _mm_sub_ps(D2, _mm_mul_ps(V02, V12));

        V00 = _mm_shuffle_ps(D0, D0, _MM_SHUFFLE(1, 1, 0, 0)); // D0Y, D0Y, D0X, D0X
        V10 = _mm_shuffle_ps(D0, D0, _MM_SHUFFLE(3, 2, 3, 2)); // D0W, D0Z, D0W, D0Z
        V01 = _mm_shuffle_ps(D1, D1, _MM_SHUFFLE(1, 0, 1, 0));
        V11 = _mm_shuffle_ps(D1, D1, _MM_SHUFFLE(3, 2, 3, 2));
        V02 = _mm_shuffle_ps(D2, D2, _MM_SHUFFLE(1, 0, 1, 0));
        V12 = _mm_shuffle_ps(D2, D2, _MM_SHUFFLE(3, 2, 3, 2));

        __m128 C0 = _mm_mul_ps(V00, V10);
        __m128 C2 = _mm_mul_ps(V01, V11);
        __m128 C4 = _mm_mul_ps(V02, V12);

        V00 = _mm_shuffle_ps(D0, D0, _MM_SHUFFLE(0, 1, 0, 1));
        V10 = _mm_shuffle_ps(D0, D0, _MM_SHUFFLE(2, 3, 2, 3));
        V01 = _mm_shuffle_ps(D1, D1, _MM_SHUFFLE(0, 1, 0, 1));
        V11 = _mm_shuffle_ps(D1, D1, _MM_SHUFFLE(2, 3, 2, 3));
        V02 = _mm_shuffle_ps(D2, D2, _MM_SHUFFLE(0, 1, 0, 1));
        V12 = _mm_shuffle_ps(D2, D2, _MM_SHUFFLE(2, 3, 2, 3));

        C0 = _mm_sub_ps(C0, _mm_mul_ps(V00, V10));
        C2 = _mm_sub_ps(C2, _mm_mul_ps(V01, V11));
        C4 = _mm_sub_ps(C4, _mm_mul_ps(V02, V12));

        __m128 C1 = _mm_sub_ps(_mm_mul_ps(V00, V12), _mm_mul_ps(V02, V10));
        C1 = _mm_add_ps(C1, _mm_sub_ps(_mm_mul_ps(V01, V10), _mm_mul_ps(V00, V11)));

        __m128 C3 = _mm_sub_ps(_mm_mul_ps(V02, V11), _mm_mul_ps(V01, V12));
        C3 = _mm_add_ps(C3, _mm_sub_ps(_mm_mul_ps(V02, V10), _mm_mul_ps(V00, V12)));

        __m128 C5 = _mm_sub_ps(_mm_mul_ps(V01, V10), _mm_mul_ps(V00, V11));
        C5 = _mm_add_ps(C5, _mm_sub_ps(_mm_mul_ps(V01, V12), _mm_mul_ps(V02, V11)));

        V00 = _mm_shuffle_ps(row1, row1, _MM_SHUFFLE(2, 3, 0, 1));
        V10 = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(2, 3, 0, 1));
        V01 = _mm_shuffle_ps(row3, row3, _MM_SHUFFLE(2, 3, 0, 1));
        V11 = _mm_shuffle_ps(row2, row2, _MM_SHUFFLE(2, 3, 0, 1));

        V02 = _mm_mul_ps(row1, C0);
        V12 = _mm_mul_ps(row0, C0);
        __m128 V03 = _mm_mul_ps(row1, C1);
        __m128 V13 = _mm_mul_ps(row0, C1);
        __m128 V04 = _mm_mul_ps(row1, C2);
        __m128 V14 = _mm_mul_ps(row0, C2);

        V02 = _mm_add_ps(V02, _mm_mul_ps(V00, C3));
        V12 = _mm_add_ps(V12, _mm_mul_ps(V10, C3));
        V02 = _mm_add_ps(V02, _mm_mul_ps(row3, C4));
        V12 = _mm_add_ps(V12, _mm_mul_ps(row2, C4));
        V03 = _mm_sub_ps(V03, _mm_mul_ps(V00, C2));
        V13 = _mm_sub_ps(V13, _mm_mul_ps(V10, C2));
        V03 = _mm_add_ps(V03, _mm_mul_ps(V01, C5));
        V13 = _mm_add_ps(V13, _mm_mul_ps(V11, C5));
        V04 = _mm_add_ps(V04, _mm_mul_ps(V00, C5));
        V14 = _mm_add_ps(V14, _mm_mul_ps(V10, C5));
        V04 = _mm_add_ps(V04, _mm_mul_ps(row3, C1));
        V14 = _mm_add_ps(V14, _mm_mul_ps(row2, C1));

        // Det computation
        __m128 det = _mm_mul_ps(row0, V02);
        det = _mm_add_ps(det, _mm_mul_ps(row1, V12));
        det = _mm_add_ps(det, _mm_mul_ps(row2, V03));
        det = _mm_add_ps(det, _mm_mul_ps(row3, V13));
        det = _mm_add_ps(det, _mm_mul_ps(row0, V04));
        det = _mm_add_ps(det, _mm_mul_ps(row1, V14));

        // Horizontal add for total determinant
        det = _mm_add_ps(det, _mm_movehl_ps(det, det));
        det = _mm_add_ss(det, _mm_shuffle_ps(det, det, 1));

        // Calculate 1.0 / Det
        // Note: If det is 0, this results in Infinity/NaN.
        // We use a safe check.
        float d = _mm_cvtss_f32(det);
        if (std::abs(d) < 1e-8f) return identity();

        __m128 invDet = _mm_div_ps(_mm_set1_ps(1.0f), det);

        Mat4 res;
        res.cols[0] = _mm_mul_ps(_mm_xor_ps(V02, _mm_setr_ps(0.0f, -0.0f, 0.0f, -0.0f)), invDet);
        res.cols[1] = _mm_mul_ps(_mm_xor_ps(V12, _mm_setr_ps(-0.0f, 0.0f, -0.0f, 0.0f)), invDet);
        res.cols[2] = _mm_mul_ps(_mm_xor_ps(V03, _mm_setr_ps(0.0f, -0.0f, 0.0f, -0.0f)), invDet);
        res.cols[3] = _mm_mul_ps(_mm_xor_ps(V13, _mm_setr_ps(-0.0f, 0.0f, -0.0f, 0.0f)), invDet);
        return res;
    }

    inline float determinant() const {
        // Reuse the logic from Inverse to compute determinant or use optimized version:
        // Swizzle method (Faster if we only want Det, but for brevity here using standard Laplace)
        float SubFactor00 = data[10] * data[15] - data[11] * data[14];
        float SubFactor01 = data[6] * data[15] - data[7] * data[14];
        float SubFactor02 = data[6] * data[11] - data[7] * data[10];
        float SubFactor03 = data[2] * data[15] - data[3] * data[14];
        float SubFactor04 = data[2] * data[11] - data[3] * data[10];
        float SubFactor05 = data[2] * data[7] - data[3] * data[6];

        return (data[0] * ( data[5] * SubFactor00 - data[9] * SubFactor01 + data[13] * SubFactor02))
             - (data[4] * ( data[1] * SubFactor00 - data[9] * SubFactor03 + data[13] * SubFactor04))
             + (data[8] * ( data[1] * SubFactor01 - data[5] * SubFactor03 + data[13] * SubFactor05))
             - (data[12]* ( data[1] * SubFactor02 - data[5] * SubFactor04 + data[9] * SubFactor05));
    }

    // --- Projections ---
    static inline Mat4 orthographic(float left, float right, float bottom, float top, float zNear, float zFar) {
        Mat4 m;
        // We set to zero first to ensure no garbage
        m.cols[0] = _mm_setzero_ps(); m.cols[1] = _mm_setzero_ps(); m.cols[2] = _mm_setzero_ps(); m.cols[3] = _mm_setzero_ps();

        float rl = 1.0f / (right - left);
        float tb = 1.0f / (top - bottom);
        float fn = 1.0f / (zFar - zNear);

        m.cols[0] = _mm_setr_ps(2.0f * rl, 0.0f, 0.0f, 0.0f);
        m.cols[1] = _mm_setr_ps(0.0f, 2.0f * tb, 0.0f, 0.0f);
        m.cols[2] = _mm_setr_ps(0.0f, 0.0f, -2.0f * fn, 0.0f);
        m.cols[3] = _mm_setr_ps(-(right + left) * rl, -(top + bottom) * tb, -(zFar + zNear) * fn, 1.0f);
        return m;
    }

    static inline Mat4 perspective(float fovRad, float aspect, float zNear, float zFar) {
        float tanHalfFovy = std::tan(fovRad * 0.5f);
        Mat4 m;
        m.cols[0] = _mm_setzero_ps(); m.cols[1] = _mm_setzero_ps(); m.cols[2] = _mm_setzero_ps(); m.cols[3] = _mm_setzero_ps();

        m.cols[0] = _mm_setr_ps(1.0f / (aspect * tanHalfFovy), 0, 0, 0);
        m.cols[1] = _mm_setr_ps(0, 1.0f / (tanHalfFovy), 0, 0);
        m.cols[2] = _mm_setr_ps(0, 0, -(zFar + zNear) / (zFar - zNear), -1.0f);
        m.cols[3] = _mm_setr_ps(0, 0, -(2.0f * zFar * zNear) / (zFar - zNear), 0);
        return m;
    }

    // --- Helpers ---
    static inline Mat4 translate(const Vec3& v) {
        Mat4 m = identity();
        m.cols[3] = _mm_setr_ps(v.x, v.y, v.z, 1.0f);
        return m;
    }
    static inline Mat4 scale(const Vec3& v) {
        Mat4 m;
        m.cols[0] = _mm_setr_ps(v.x, 0, 0, 0);
        m.cols[1] = _mm_setr_ps(0, v.y, 0, 0);
        m.cols[2] = _mm_setr_ps(0, 0, v.z, 0);
        m.cols[3] = _mm_setr_ps(0, 0, 0, 1);
        return m;
    }
    static inline Mat4 rotate(const Quat& q) {
        return Mat4(Vec3(0,0,0), q, Vec3(1,1,1));
    }

    static inline Mat4 view_from_transform(const Mat4& transform) {
        return inverse_affine(transform);
    }
};

#endif //GAME_MAT4_H