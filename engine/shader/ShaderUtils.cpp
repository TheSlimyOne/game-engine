// ShaderUtils.cpp

#include "ShaderUtils.h"


#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

namespace ShaderUtils {

bgfx::ShaderHandle load_shader_bin(const std::string& file_path)
{
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open shader: " << file_path << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    const std::streamsize file_size = file.tellg();
    if (file_size <= 0) {
        std::cerr << "Shader is empty or unreadable: " << file_path << std::endl;
        return BGFX_INVALID_HANDLE;
    }
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(file_size) + 1u);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), file_size)) {
        std::cerr << "Failed to read shader: " << file_path << std::endl;
        return BGFX_INVALID_HANDLE;
    }
    buffer[static_cast<size_t>(file_size)] = '\0';

    const bgfx::Memory* mem = bgfx::copy(buffer.data(),
                                         static_cast<uint32_t>(buffer.size()));
    return bgfx::createShader(mem);
}

bgfx::ProgramHandle load_compute_shader(const std::string& cs_path,  bool destroyShaders)
{
    auto csh = load_shader_bin(cs_path);
    if (!bgfx::isValid(csh)) {
        std::cerr << "Program shaders invalid: CS=" << cs_path << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    bgfx::ProgramHandle prog = bgfx::createProgram(csh, destroyShaders);
    if (!bgfx::isValid(prog)) {
        bgfx::destroy(csh);
        std::cerr << "Failed to create program: VS=" << cs_path << std::endl;
    }
    return prog;
}

bgfx::ProgramHandle load_program(const std::string& vs_path, const std::string& fs_path, bool destroyShaders) {
    auto vsh = load_shader_bin(vs_path);
    auto fsh = load_shader_bin(fs_path);
    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        if (bgfx::isValid(vsh)) bgfx::destroy(vsh);
        if (bgfx::isValid(fsh)) bgfx::destroy(fsh);
        std::cerr << "Program shaders invalid: VS=" << vs_path
                  << " FS=" << fs_path << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    bgfx::ProgramHandle prog = bgfx::createProgram(vsh, fsh, destroyShaders);
    if (!bgfx::isValid(prog)) {
        bgfx::destroy(vsh);
        bgfx::destroy(fsh);
        std::cerr << "Failed to create program: VS=" << vs_path
                  << " FS=" << fs_path << std::endl;
    }
    return prog;
}

float* get_time_uniform()
{
    static const auto t0 = std::chrono::high_resolution_clock::now();
    const auto now = std::chrono::high_resolution_clock::now();
    const float seconds = std::chrono::duration<float>(now - t0).count();
    static float timeVec4[4] = { seconds, 0.0f, 0.0f, 0.0f };
    return timeVec4;
}

} // namespace ShaderUtils