//
// Created by Jordan on 1/15/2026.
//

#ifndef GAME_TESSKEY_H
#define GAME_TESSKEY_H

#pragma once
#include <cstdint>
#include <iostream>

#include "math/Common.h"


struct TessKey final {
    std::uint64_t data;
    std::uint32_t meshPolygonID;
    std::uint32_t meshData; // FFF0000000000000000000000000RRRR

    [[nodiscard]] uint32_t flags()     const { return meshData >> 29; }
    [[nodiscard]] uint32_t root_id()    const { return meshData & 0xF; }

    [[nodiscard]] uint32_t msb()    const { return data >> 32; }
    [[nodiscard]] uint32_t lsb()    const { return data & 0xFFFFFFFF; }


    TessKey(std::uint64_t data_, uint32_t meshPolygonID_, uint32_t meshData_);

    TessKey() = default;
    ~TessKey() = default;

    [[nodiscard]] Mat3 create_leaf_space_to_quadtree_space() const;

    [[nodiscard]] static Vec2 get_translation(uint32_t b1);
    [[nodiscard]] static Vec2 rotate(int rotationIndex, const Vec2& translation);
    [[nodiscard]] static int get_rotation(uint32_t b1b2);
    [[nodiscard]] uint32_t  get_branching(int level) const;
    [[nodiscard]] uint32_t  get_most_significant_bit() const;
    [[nodiscard]] static Vec2 quick_pi2(int a);
};

std::ostream& operator<<(std::ostream& os, const TessKey& key);


#endif //GAME_TESSKEY_H