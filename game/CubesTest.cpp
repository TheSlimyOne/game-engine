// test.cpp

#include <Engine.h>
#include <iomanip>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <core/Paths.h>

// Configuration
constexpr int CUBE_COUNT = 1000;


struct State {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float time = 0.0f;

    // Logic flag to ensure we only generate cubes once
    bool initialized = false;

    // State storage moved here
    std::vector<Entity> cubes;
    std::vector<Vec3> initial_positions;
    std::vector<float> move_offsets;
};

void cubes_update(World& world, float dt, State& data) {

    // ---------------------------------------------------------
    // 1. GENERATION (First Frame Only)
    // ---------------------------------------------------------
    if (!data.initialized) {
        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_real_distribution<float> distRadius(10.0f, 200.0f);
        std::uniform_real_distribution<float> distTheta(0.0f, 6.28318f);
        std::uniform_real_distribution<float> distZ(-1.0f, 1.0f);
        std::uniform_real_distribution<float> distRot(0.0f, 6.28318f);
        std::uniform_real_distribution<float> distOffset(0.0f, 10.0f);

        data.cubes.reserve(CUBE_COUNT);
        data.initial_positions.reserve(CUBE_COUNT);
        data.move_offsets.reserve(CUBE_COUNT);

        for(int i = 0; i < CUBE_COUNT; ++i) {
            Entity cube = world.create_entity();
            world.add_component(cube, ComponentType::Transform);
            world.add_component(cube, ComponentType::Model);

            world.load_mesh(cube, (ROOT / "models/Cube.fbx").string());
            world.load_material(cube, "color");

            float r = distRadius(gen);
            float u = distZ(gen);
            float theta = distTheta(gen);
            float val = sqrt(1.0f - u * u);

            float x = r * val * cos(theta);
            float y = r * u;
            float z = r * val * sin(theta);

            Vec3 pos = Vec3(x, y, z);
            world.set_position(cube, pos);

            // Store in State struct
            data.cubes.push_back(cube);
            data.initial_positions.push_back(pos);
            data.move_offsets.push_back(distOffset(gen));

            world.set_rotation(cube, Vec3(distRot(gen), distRot(gen), distRot(gen)));
        }

        data.initialized = true;
    }

    // ---------------------------------------------------------
    // 2. UPDATE LOGIC
    // ---------------------------------------------------------
    // Animate Cubes
    float wave_speed = 2.0f;
    float wave_height = 5.0f;

    for (size_t i = 0; i < data.cubes.size(); ++i) {
        Vec3 base_pos = data.initial_positions[i];
        float offset = data.move_offsets[i];

        float new_y = base_pos.y + sin(data.time * wave_speed + offset) * wave_height;

        world.set_position(data.cubes[i], Vec3(base_pos.x, new_y, base_pos.z));
    }

}

// SYSTEMS_ON_STARTUP(cubes_start);
// SYSTEMS_ON_UPDATE(cubes_update, State);