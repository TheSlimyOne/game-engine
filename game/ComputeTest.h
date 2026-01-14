#ifndef GAME_COMPUTETEST_H
#define GAME_COMPUTETEST_H


#pragma once

#include <bgfx/bgfx.h>
#include <filesystem>
#include <string_view>

class ComputeTest
{
public:
    ComputeTest();
    ~ComputeTest();

    // Initialize GPU resources (call once)
    void Initialize();

    // Run compute → fullscreen render (call each frame)
    void Update();

private:
    // internal helpers
    static bgfx::ShaderHandle LoadShader(const std::filesystem::path& path);
    void ClearCounterToZero() const;
    void DispatchCompute();

private:
    // gpu resources
    bgfx::UniformHandle m_uTex0{ BGFX_INVALID_HANDLE };
    bgfx::TextureHandle m_counterTex = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_computeProgram{ BGFX_INVALID_HANDLE };

    bool m_dispatched = false;
    bool m_pendingRead = false;
    uint32_t m_readbackValue = 0;
    bool m_initialized = false;
};

#endif //GAME_COMPUTETEST_H