//
// Created by Jordan on 1/14/2026.
//

#include "PlanetRenderingPipeline.h"

#include <bitset>
#include <filesystem>
#include <iostream>

#include "TessKey.h"
// #include <bx/math.h>
#include "core/Paths.h"
#include "shader/ShaderUtils.h"

PlanetRenderingPipeline::PlanetRenderingPipeline() = default;
PlanetRenderingPipeline::~PlanetRenderingPipeline() = default;




struct RenderInstance
{
    float mtx[16]  = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    float color[4] = {}; // rgba

    inline static bgfx::VertexLayout ms_layout{};
    static void init()
    {
        ms_layout
            .begin()
            .add(bgfx::Attrib::TexCoord0, 4, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord1, 4, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord2, 4, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord3, 4, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Float)
            .end();
    }
};

void PlanetRenderingPipeline::initialize()
{
    if (m_initialized)
        return;

    m_init_tessellation_pass = ShaderUtils::load_compute_shader(
        DIR_INIT_TESS,
        true);

    m_prep_tessellation_pass = ShaderUtils::load_compute_shader(
        DIR_PREP_TESS,
        true);

    m_exec_tessellation_pass = ShaderUtils::load_compute_shader(
        DIR_EXEC_TESS,
        true);

    m_draw_program = ShaderUtils::load_program(
        DIR_VERTEX_SHADER, DIR_FRAGMENT_SHADER,
        true
    );

    bgfx::setViewName(VIEW_MAIN, "Planet Rendering");


    // ========= Initialize Tesselation Pass Resources ========= //

    u_init_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4);
    {
        // primitive_count_full
        // primitive_count_culled
        // primitive_count_rendered
        // padding
        bgfx::VertexLayout layout;
        layout.begin().add(bgfx::Attrib::TexCoord4, 4, bgfx::AttribType::Float).end();
        u_atomic_counter = bgfx::createDynamicVertexBuffer(4, layout,BGFX_BUFFER_COMPUTE_READ_WRITE);
    }

    {
        // read_index
        // write_index
        // delete_index
        // padding
        bgfx::VertexLayout layout;
        layout.begin().add(bgfx::Attrib::TexCoord4, 4, bgfx::AttribType::Float).end();
        u_indices = bgfx::createDynamicVertexBuffer(3, layout,BGFX_BUFFER_COMPUTE_READ_WRITE);
    }

    {
        u_exec_indirect_buffer = bgfx::createIndirectBuffer(1);
    }

    {
        u_draw_command_buffer = bgfx::createIndirectBuffer(MAX_KEYS);

        const bgfx::Memory* mem = bgfx::alloc(sizeof(uint32_t) );
        *reinterpret_cast<uint32_t*>(mem->data) = 0;
        u_draw_indirect_count_buffer = bgfx::createIndexBuffer(mem, BGFX_BUFFER_INDEX32 | BGFX_BUFFER_COMPUTE_WRITE | BGFX_BUFFER_DRAW_INDIRECT);

        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::TexCoord0, 4, bgfx::AttribType::Float) // transform row 0
            .add(bgfx::Attrib::TexCoord1, 4, bgfx::AttribType::Float) // row 1
            .add(bgfx::Attrib::TexCoord2, 4, bgfx::AttribType::Float) // row 2
            .add(bgfx::Attrib::TexCoord3, 4, bgfx::AttribType::Float) // row 3
        .end();

        u_draw_instance_buffer = bgfx::createDynamicVertexBuffer(MAX_KEYS, layout, BGFX_BUFFER_COMPUTE_WRITE);
    }

    {
        const auto W = static_cast<uint16_t>(std::ceil(std::sqrt(static_cast<double>(MAX_KEYS))));
        const auto H = W;

        u_read_list = bgfx::createTexture2D(
            W, H,
            false, 1,
            bgfx::TextureFormat::RGBA32U,
            BGFX_TEXTURE_COMPUTE_WRITE,
            generate_keys(W, H)
        );
    }

    {
        struct PosVertex { float x, y, z; };

        static const PosVertex kCubeVertices[] =
        {
            // +X
            { 1,-1,-1 }, { 1, 1,-1 }, { 1, 1, 1 },

            { 1,-1, 1 },
            // -X
            {-1,-1, 1 }, {-1, 1, 1 }, {-1, 1,-1 }, {-1,-1,-1 },
            // +Y
            {-1, 1,-1 }, {-1, 1, 1 }, { 1, 1, 1 }, { 1, 1,-1 },
            // -Y
            {-1,-1, 1 }, {-1,-1,-1 }, { 1,-1,-1 }, { 1,-1, 1 },
            // +Z
            {-1,-1, 1 }, { 1,-1, 1 }, { 1, 1, 1 }, {-1, 1, 1 },
            // -Z
            { 1,-1,-1 }, {-1,-1,-1 }, {-1, 1,-1 }, { 1, 1,-1 },
        };

        const bgfx::Memory* vbMem = bgfx::copy(kCubeVertices, sizeof(kCubeVertices));

        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            // .add(bgfx::Attrib::Normal,   3, bgfx::AttribType::Float)
            // .add(bgfx::Attrib::TexCoord0,2, bgfx::AttribType::Float)
            .end();
        u_vertex_buffer = bgfx::createVertexBuffer(vbMem, layout);
    }

    {
        static const uint32_t kCubeIndices[] =
        {
            0, 1, 2,
            0, 2, 3,        // +X
           4, 5, 6,  4, 6, 7,        // -X
            8, 9,10,  8,10,11,        // +Y
           12,13,14, 12,14,15,        // -Y
           16,17,18, 16,18,19,        // +Z
           20,21,22, 20,22,23,        // -Z
        };

        const bgfx::Memory* ibMem = bgfx::copy(kCubeIndices,  sizeof(kCubeIndices));

        u_index_buffer = bgfx::createIndexBuffer(ibMem, BGFX_BUFFER_INDEX32);
    }

    {
        u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
    }

    {
        struct InstanceData
        {
            float vertexOffset = 0;
            float vertexCount  = 0;
            float indexOffset  = 0;
            float indexCount   = 0;
        };

        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::TexCoord0, 4, bgfx::AttribType::Float)
        .end();

        const bgfx::Memory* mem = bgfx::alloc(sizeof(InstanceData));
        auto* data = reinterpret_cast<InstanceData*>(mem->data);

        data[0].vertexOffset = 0;
        data[0].vertexCount  = 24;
        data[0].indexOffset  = 0;
        data[0].indexCount   = 36;

        u_instance_data = bgfx::createVertexBuffer(mem, layout, BGFX_BUFFER_COMPUTE_READ);
    }

    // ========= Prepare Tessellation Pass Resources ========= //


    // ========= Execute Tessellation Pass Resources ========= //


    m_initialized = true;
}

void PlanetRenderingPipeline::dispatch_compute()
{
    if (!m_initialized)
        initialize();

    // if (m_restart)
    // {
    //     constexpr float params[4] = {4}; // u_starting_count
    //     bgfx::setUniform(u_init_params, params);
    //
    //     bgfx::setBuffer(0, u_atomic_counter, bgfx::Access::ReadWrite);
    //     bgfx::setBuffer(1, u_indices, bgfx::Access::ReadWrite);
    //     bgfx::setBuffer(2, u_exec_indirect_buffer, bgfx::Access::ReadWrite);
    //     bgfx::dispatch(VIEW_MAIN, m_init_tessellation_pass, 1, 1, 1);
    //
    //     m_restart = false;
    // }

    // bgfx::setBuffer(0, u_atomic_counter, bgfx::Access::ReadWrite);
    // bgfx::setBuffer(1, u_indices, bgfx::Access::ReadWrite);
    // bgfx::setBuffer(2, u_exec_indirect_buffer, bgfx::Access::ReadWrite);
    // bgfx::setBuffer(3, u_draw_indirect_count_buffer, bgfx::Access::ReadWrite);
    // bgfx::dispatch(VIEW_MAIN, m_prep_tessellation_pass, 1, 1, 1);



    bgfx::setBuffer(0, u_atomic_counter, bgfx::Access::ReadWrite);
    bgfx::setBuffer(1, u_indices, bgfx::Access::ReadWrite);
    bgfx::setImage (2, u_read_list, 0, bgfx::Access::ReadWrite, bgfx::TextureFormat::RGBA32U);
    bgfx::setBuffer(3, u_instance_data, bgfx::Access::Read);
    bgfx::setBuffer(4, u_draw_command_buffer, bgfx::Access::Write);
    bgfx::setBuffer(5, u_draw_instance_buffer, bgfx::Access::Write);
    bgfx::setBuffer(6, u_draw_indirect_count_buffer, bgfx::Access::ReadWrite);
    bgfx::dispatch(VIEW_MAIN, m_exec_tessellation_pass, 1,1,1);

    bgfx::setState(BGFX_STATE_DEFAULT);
    bgfx::setUniform(u_time, ShaderUtils::get_time_uniform());
    bgfx::setIndexBuffer(u_index_buffer);
    bgfx::setVertexBuffer(0, u_vertex_buffer);
    bgfx::setInstanceDataBuffer(u_draw_instance_buffer, 0, MAX_KEYS);
    bgfx::submit(VIEW_MAIN, m_draw_program, u_draw_command_buffer, 0, u_draw_indirect_count_buffer);


    m_pendingRead = true;
    m_dispatched = true;
}

const bgfx::Memory* PlanetRenderingPipeline::generate_keys(const uint16_t width, const uint16_t height)
{
    const uint32_t texelCount = static_cast<uint32_t>(width) * static_cast<uint32_t>(height);
    std::vector<uint32_t> rgba;
    rgba.resize(texelCount * 4u, 0u);

    constexpr uint32_t amount = 1u << (2u * 1u); // lod=1 => 4

    uint32_t texelIndex = 0;
    for (uint32_t i = 0; i < 6 && texelIndex < texelCount; ++i)
    {
        for (uint32_t j = 0; j < amount && texelIndex < texelCount; ++j)
        {
            for (uint32_t k = 0; k < 4 && texelIndex < texelCount; ++k)
            {
                const TessKey key(amount + j, i, k);

                rgba[texelIndex * 4u + 0u] = key.msb();
                rgba[texelIndex * 4u + 1u] = key.lsb();
                rgba[texelIndex * 4u + 2u] = key.meshPolygonID;
                rgba[texelIndex * 4u + 3u] = key.meshData;

                ++texelIndex;
            }
        }
    }

    return bgfx::copy(rgba.data(), static_cast<uint32_t>(rgba.size() * sizeof(uint32_t)));
}
