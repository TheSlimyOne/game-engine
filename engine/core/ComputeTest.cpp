#include "ComputeTest.h"

#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <iostream>
#include <core/Paths.h>
#include <bgfx/bgfx.h>

static constexpr uint16_t TEX_W = 512;
static constexpr uint16_t TEX_H = 512;

static constexpr bgfx::ViewId VIEW_COMPUTE = 0;

ComputeTest::ComputeTest() = default;

ComputeTest::~ComputeTest()
{
    if (bgfx::isValid(m_uTex0))              bgfx::destroy(m_uTex0);
    if (bgfx::isValid(m_counterTex))         bgfx::destroy(m_counterTex);
    if (bgfx::isValid(m_computeProgram))     bgfx::destroy(m_computeProgram);
}

bgfx::ShaderHandle ComputeTest::LoadShader(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("Failed to open shader: " + path.string());

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    const bgfx::Memory* mem = bgfx::alloc(static_cast<uint32_t>(size));
    file.read(reinterpret_cast<char*>(mem->data), size);

    return bgfx::createShader(mem);
}

void ComputeTest::Initialize()
{
    if (m_initialized)
        return;

    const std::filesystem::path shaders = ROOT / "shaders/test_shader";

    m_computeProgram = bgfx::createProgram(
       LoadShader((shaders / "cs_test_shader.bin").string()),
       true
   );

    constexpr uint32_t zero = 0;
    const bgfx::Memory* mem = bgfx::copy(&zero, sizeof(zero));

    m_counterTex = bgfx::createTexture2D(
        1, 1,
        false, 1,
        bgfx::TextureFormat::R32U,
        BGFX_TEXTURE_COMPUTE_WRITE,
        mem
    );

    // compute view
    bgfx::setViewName(VIEW_COMPUTE, "Compute View");

    m_initialized = true;
}

void ComputeTest::DispatchCompute()
{
    if (!bgfx::isValid(m_counterTex) || !bgfx::isValid(m_computeProgram))
        return;

    if (!m_dispatched)
    {
        ClearCounterToZero();

        // Bind as image slot 0, readwrite for atomics
        bgfx::setImage(
            0,
            m_counterTex,
            0,
            bgfx::Access::ReadWrite,
            bgfx::TextureFormat::R32U
        );

        bgfx::dispatch(VIEW_COMPUTE, m_computeProgram, 1, 1, 1);

        m_pendingRead = true;
        bgfx::readTexture(m_counterTex, &m_readbackValue);

        m_dispatched = true;
    }
    else if (m_pendingRead)
    {
        if (m_readbackValue != 0)
        {
            std::cout << "[ComputeCounterTest] counter=" << m_readbackValue << "\n";
            m_pendingRead = false;
        }
    }

    bgfx::touch(VIEW_COMPUTE);
}

void ComputeTest::ClearCounterToZero() const
{
    // Re-upload 0 into the 1x1 texture before dispatch.
    constexpr uint32_t zero = 0;
    const bgfx::Memory* mem = bgfx::copy(&zero, sizeof(zero));
    bgfx::updateTexture2D(m_counterTex, 0, 0, 0, 0, 1, 1, mem);
}

void ComputeTest::Update()
{
    if (!m_initialized)
        Initialize();

    bgfx::touch(VIEW_COMPUTE);

    DispatchCompute();
}
