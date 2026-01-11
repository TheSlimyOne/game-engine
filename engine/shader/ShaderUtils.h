// ShaderUtils.h

#ifndef GAME_SHADERUTILS_H
#define GAME_SHADERUTILS_H
#include <bgfx/bgfx.h>
#include <string>

namespace ShaderUtils {

    // Loads a single compiled shader .bin file.
    bgfx::ShaderHandle load_shader_bin(const std::string& file_path);

    // Convenience helper to create a program from vs/fs paths.
    // If creation fails, returns BGFX_INVALID_HANDLE.
    bgfx::ProgramHandle load_program(const std::string& vs_path, const std::string& fs_path, bool destroyShaders = true);
}

#endif //GAME_SHADERUTILS_H