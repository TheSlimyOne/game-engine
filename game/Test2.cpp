// test.cpp

#include <Engine.h>
#include <iomanip>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <core/Paths.h>

void start_test(World& world) {
    // 3. Create cube
    Entity cube = world.create_entity();
    world.add_component(cube, ComponentType::Transform);
    world.add_component(cube, ComponentType::Model);
    world.load_mesh(cube, (ROOT / "models/Kirby/Kirby.fbx").string());
    world.load_material(cube, "cel");
    world.load_texture(cube, (ROOT / "models/Kirby/Kirby_BaseColor.png").string());


    world.set_rotation(cube, Vec3(1.0, 0.0, 0.0), radians(90.0));
    world.set_scale(cube, Vec3(3.0f, 3.0f, 3.0f));
    world.set_position(cube, Vec3(0.0f, 5.0f, 0.0f));
    world.select_entity(cube);
}



SYSTEMS_ON_STARTUP(start_test);
