//
// Created by Jordan on 1/15/2026.
//

#include "TessKey.h"

#include <array>
#include <bit>
#include <bitset>
#include <cstdint>
#include <iostream>
#include "math/Common.h"
#include "math/Mat3.h"
#include "math/Vec2.h"

TessKey::TessKey(const uint64_t data_, const std::uint32_t meshPolygonID_, const std::uint32_t meshData_)
    : data(data_), meshPolygonID(meshPolygonID_), meshData(meshData_) {}

Mat3 TessKey::create_leaf_space_to_quadtree_space() const
{
    const uint32_t msb_index = get_most_significant_bit();
    Vec2 translation = {0, 0};

    int theta = 0;
    float scale = 1;

    for (int i = 0; i < msb_index / 2; i++)
    {
        // get the last 2 bits of the number
        const uint32_t b1b2 = get_branching(i);
        const uint32_t b1 = b1b2 >> 1;

        Vec2 temp = get_translation(b1) * scale * 0.5f;
        translation = translation + rotate(theta, temp);
        theta += get_rotation(b1b2);
        scale *= 0.5f;
    }

    const Vec2 trig = quick_pi2(theta);

    const Mat3 quadtreeSpace(std::array{
        trig.x * scale, -trig.y * scale, translation.x,
        trig.y * scale,  trig.x * scale, translation.y,
        0.0f,            0.0f,            1.0f
    }.data());

    return quadtreeSpace;
}

uint32_t TessKey::get_most_significant_bit() const
{
    uint64_t uint64 = data;
    int msb_index = 0;

    while (uint64 > 1)
    {
        uint64 >>= 1;
        msb_index++;
    }

    return msb_index;
}

uint32_t TessKey::get_branching(const int level) const
{
    const uint32_t shiftAmount = get_most_significant_bit() - 2 - (level * 2);
    return (data >> shiftAmount) & 0x3;
}

Vec2 TessKey::get_translation(const uint32_t b1)
{
    return {static_cast<float>(b1 & 0x1), static_cast<float>(b1 ^ 0x1)};
}

Vec2 TessKey::rotate(const int rotationIndex, const Vec2& translation)
{
    const Vec2 trig = quick_pi2(rotationIndex);

    return {
        trig.x * translation.x - trig.y * translation.y,
        trig.y * translation.x + trig.x * translation.y
    };
}

int TessKey::get_rotation(const uint32_t b1b2)
{
    const uint32_t b1 = b1b2 >> 1;
    const uint32_t b2 = b1b2 & 1;

    const uint32_t a = b1b2 ^ 0x2;
    const uint32_t b = a | 0x1;
    const uint32_t c = b1 ^ b2;
    return b * c;
}

Vec2 TessKey::quick_pi2(const int a)
{
    const int b = a & 3;
    const int b1 = b >> 1;
    const int b2 = b & 1;
    const int bn2 = b2 ^ 1;
    const int c = bn2 - (2 * (b1 & bn2));
    const int s = b2 - (2 * (b1 & b2));
    return Vec2(c,s);
}

static std::string ToBase4(uint64_t v)
{
    if (v == 0)
        return "0";

    std::string s;
    while (v > 0)
    {
        const uint64_t digit = v % 4ull;
        s.push_back(char('0' + digit));
        v /= 4ull;
    }

    // reverse
    for (size_t i = 0, j = s.size() - 1; i < j; ++i, --j)
        std::swap(s[i], s[j]);

    return s;
}

std::ostream& operator<<(std::ostream& os, const TessKey& key)
{
    const std::string dataBase4 = ToBase4(key.data);


    return os << "Key: " << dataBase4;
}