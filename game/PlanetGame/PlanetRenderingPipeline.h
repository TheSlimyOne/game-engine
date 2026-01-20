//
// Created by Jordan on 1/14/2026.
//

#ifndef GAME_PLANETRENDERINGPIPELINE_H
#define GAME_PLANETRENDERINGPIPELINE_H

#pragma once
#include <filesystem>
#include <bgfx/bgfx.h>

#include "core/Paths.h"

inline const std::string DIR_INIT_TESS       = (ROOT/"shaders"/"planet_game_shaders"/ "planet_rendering_pipeline_shaders"/"cs_init_tess.bin").string();
inline const std::string DIR_PREP_TESS       = (ROOT/"shaders"/"planet_game_shaders"/ "planet_rendering_pipeline_shaders"/"cs_prep_tess.bin").string();
inline const std::string DIR_EXEC_TESS       = (ROOT/"shaders"/"planet_game_shaders"/ "planet_rendering_pipeline_shaders"/"cs_exec_tess.bin").string();

inline const std::string DIR_VERTEX_SHADER   = (ROOT/"shaders"/"planet_game_shaders"/ "planet_rendering_pipeline_shaders"/"vs_planet.bin").string();
inline const std::string DIR_FRAGMENT_SHADER = (ROOT/"shaders"/"planet_game_shaders"/ "planet_rendering_pipeline_shaders"/"fs_planet.bin").string();
constexpr uint16_t MAX_KEYS = 1000;

class PlanetRenderingPipeline
{

public:
    PlanetRenderingPipeline();
    ~PlanetRenderingPipeline();

    void initialize();
    void dispatch_compute();

private:
    static const bgfx::Memory* generate_keys(uint16_t width, uint16_t height);


    uint32_t m_readbackFrame;
    bool m_dispatched = false;
    bool m_pendingRead = false;
    bool m_initialized = false;
    bool m_restart = true;

    static constexpr bgfx::ViewId VIEW_MAIN = 0;

    bgfx::ProgramHandle               m_init_tessellation_pass      { BGFX_INVALID_HANDLE };
    bgfx::ProgramHandle               m_prep_tessellation_pass      { BGFX_INVALID_HANDLE };
    bgfx::ProgramHandle               m_exec_tessellation_pass      { BGFX_INVALID_HANDLE };
    bgfx::ProgramHandle               m_draw_program                { BGFX_INVALID_HANDLE };

    bgfx::DynamicVertexBufferHandle   u_atomic_counter              { BGFX_INVALID_HANDLE };
    bgfx::DynamicVertexBufferHandle   u_indices                     { BGFX_INVALID_HANDLE };
    bgfx::DynamicVertexBufferHandle   u_draw_instance_buffer        { BGFX_INVALID_HANDLE };

    bgfx::UniformHandle               u_init_params                 { BGFX_INVALID_HANDLE };
    bgfx::IndirectBufferHandle        u_exec_indirect_buffer        { BGFX_INVALID_HANDLE };
    bgfx::IndirectBufferHandle        u_draw_command_buffer         { BGFX_INVALID_HANDLE };
    bgfx::IndexBufferHandle           u_draw_indirect_count_buffer  { BGFX_INVALID_HANDLE };

    bgfx::TextureHandle               u_read_list                   { BGFX_INVALID_HANDLE };

    bgfx::VertexBufferHandle          u_vertex_buffer               { BGFX_INVALID_HANDLE };
    bgfx::IndexBufferHandle           u_index_buffer                { BGFX_INVALID_HANDLE };

    bgfx::VertexBufferHandle          u_instance_data               { BGFX_INVALID_HANDLE };;
    bgfx::UniformHandle               u_time                        { BGFX_INVALID_HANDLE };;
};


#endif //GAME_PLANETRENDERINGPIPELINE_H