// MaterialManager.cpp

#include "MaterialManager.h"
#include <bgfx/bgfx.h>
#include <memory>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

#include <shader/ShaderUtils.h>
#include <materials/Material.h>
#include <materials/ColorMaterial.h>
#include <materials/CoordinatePlaneMaterial.h>
#include <materials/CelMaterial.h>


namespace fs = std::filesystem;

std::shared_ptr<Material> MaterialManager::load_from_id(const std::string &material_id) {
    fs::path shader_dir = "C:/Users/aruem/Desktop/game-engine/shaders";
    fs::path mat_dir = shader_dir / material_id;

    if (!fs::exists(mat_dir) || !fs::is_directory(mat_dir)) {
        std::cerr << "Material directory not found: " << mat_dir << std::endl;
        return nullptr;
    }

    fs::path vs_path = mat_dir / ("vs_" + material_id + ".bin");
    fs::path fs_path = mat_dir / ("fs_" + material_id + ".bin");

    if (!fs::exists(vs_path) || !fs::exists(fs_path)) {
        std::cerr << "Missing shader file(s) for material: " << material_id << std::endl;
        return nullptr;
    }

    auto vsh = ShaderUtils::load_shader_bin(vs_path.string());
    auto fsh = ShaderUtils::load_shader_bin(fs_path.string());
    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        if (bgfx::isValid(vsh)) bgfx::destroy(vsh);
        if (bgfx::isValid(fsh)) bgfx::destroy(fsh);
        std::cerr << "Material shaders are invalid for: " << material_id << std::endl;
        return nullptr;
    }

    auto prog = bgfx::createProgram(vsh, fsh, false);
    if (!bgfx::isValid(prog)) {
        bgfx::destroy(vsh);
        bgfx::destroy(fsh);
        std::cerr << "Failed to create program for: " << material_id << std::endl;
        return nullptr;
    }

    std::shared_ptr<Material> material;

    if (material_id == "color") {
        material = std::make_shared<ColorMaterial>(vsh, fsh, prog);
    } else if (material_id == "coordinate_plane") {
        material = std::make_shared<CoordinatePlaneMaterial>(vsh, fsh, prog);
    } else if (material_id == "cel") {
        material = std::make_shared<CelMaterial>(vsh, fsh, prog);
    }


    return material;
}


