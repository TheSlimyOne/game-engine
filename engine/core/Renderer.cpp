// Renderer.cpp

#include "Renderer.h"
#include "World.h"
#include "MeshData.h"


#include <materials/Material.h>
#include <shader/ShaderUtils.h>
#include <post_processing/OutlineEffect.h>
#include <components/ModelComponent.hpp>
#include <core/Types.h>
#include <platform/Window.h>

#include <iostream>

#include <bgfx/bgfx.h>
#include <bx/bx.h>
#include <bgfx/platform.h>
#include "GLFW/glfw3.h"
#include <core/Paths.h>
#include "PlanetGame/PlanetRenderingPipeline.h"

namespace
{
    constexpr bgfx::ViewId kSceneViewId         = 0; // Off-screen scene
    constexpr bgfx::ViewId kSelectionViewId     = 1; // Selection mask
    constexpr bgfx::ViewId kPostProcessViewBase = 2; // Post-process output
    constexpr bgfx::ViewId kPickingViewId       = 3; // new
    constexpr bgfx::ViewId kPickingBlitViewId   = 4;

    constexpr uint32_t kMaxInstancesPerDraw = 50'000;

    inline void encode_id_rgba8(uint32_t id, float outColor[4])
    {
        // Reserve 0 as "no entity"
        id = id & 0x00FFFFFFu;

        uint8_t r = (id      ) & 0xFF;
        uint8_t g = (id >>  8) & 0xFF;
        uint8_t b = (id >> 16) & 0xFF;

        outColor[0] = r / 255.0f;
        outColor[1] = g / 255.0f;
        outColor[2] = b / 255.0f;
        outColor[3] = 1.0f;
    }

    inline uint32_t decode_id_from_bgra(uint8_t b, uint8_t g, uint8_t r)
    {
        return (uint32_t(r)      ) |
               (uint32_t(g) << 8 ) |
               (uint32_t(b) << 16);
    }
}

Renderer::Renderer() : m_initialized(false) {}

Renderer::~Renderer() {
    shutdown();
}

// ======================== Renderer Interface ==================== //
bool Renderer::init(Window* window) {
    if (!window) {
        std::cerr << "[Renderer] ERROR: init called with null Window.\n";
        return false;
    }

    if (!init_bgfx(window)) {
        return false;
    }

    // Create off-screen render targets (scene + selection).
    create_scene_targets();
    create_selection_target();
    create_picking_target();

    // Fullscreen quad used by post processing.
    m_fullscreenQuad.init();

    // Shader program used to render the selection mask.
    init_selection_program();

    // Build the post-processing chain (OutlineEffect etc.).
    init_post_process_chain();

    if (!init_picking_program()) {    // NEW
        std::cerr << "[Renderer] WARNING: Picking program failed to init.\n";
    }

    // Initialize delta time.
    m_last_dt = glfwGetTime();

    m_initialized = true;
    return true;
}

uint32_t Renderer::get_current_frame() const
{
    return currentFrame;
}

void Renderer::shutdown() {
    if (!m_initialized) {
        return;
    }

    // Destroy post-process chain (which owns OutlineEffect).
    m_postChain.shutdown();
    m_outlineEffect = nullptr;

    m_fullscreenQuad.shutdown();

    if (bgfx::isValid(m_selectionProgram)) {
        bgfx::destroy(m_selectionProgram);
        m_selectionProgram = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_pickProgram)) {
        bgfx::destroy(m_pickProgram);
        m_pickProgram = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(u_idColor)) {
        bgfx::destroy(u_idColor);
        u_idColor = BGFX_INVALID_HANDLE;
    }

    destroy_selection_target();
    destroy_scene_targets();
    destroy_picking_target();          // NEW

    bgfx::shutdown();
    m_initialized = false;
}

float Renderer::get_delta_time() {
    double current_time = glfwGetTime();
    double delta_time = current_time - m_last_dt;
    m_last_dt = current_time;

    return static_cast<float>(delta_time);
}

void Renderer::begin_frame() {
}

void Renderer::end_frame(World& world) {
    currentFrame = bgfx::frame();

    if (m_pickState.pendingReadback &&
        currentFrame >= m_pickState.readyFrame &&
        m_pickState.active)
    {
        m_pickState.pendingReadback = false;
        m_pickState.active          = false;

        if (m_pickBuffer.size() >= 4) {
            // BGRA8 order in your decode path:
            uint8_t b = m_pickBuffer[0];
            uint8_t g = m_pickBuffer[1];
            uint8_t r = m_pickBuffer[2];

            uint32_t id = decode_id_from_bgra(b, g, r);
            Entity hit = (id == 0) ? 0 : static_cast<Entity>(id);

            world.m_pickResult.entity    = hit;
            world.m_pickResult.hasResult = true;
        } else {
            world.m_pickResult.entity    = 0;
            world.m_pickResult.hasResult = true;
        }
    }
}
PlanetRenderingPipeline planet_rendering_pipeline;
void Renderer::draw_frame(World &world) {
    Entity active_camera = world.get_active_camera();
    if (!active_camera) {
        std::cerr << "[Renderer] ERROR: No active camera set for rendering.\n";
        return;
    }

    // 1) Render the 3D scene into an off-screen framebuffer (color + depth).
    render_scene(world, active_camera);

    planet_rendering_pipeline.dispatch_compute();

    // 2) Render a black-and-white mask of selected entities into a separate RT.
    render_selection_mask(world, active_camera);
    render_picking(world, active_camera);   // NEW

    // 3) Run post-processing (outline effect, etc.) and present to backbuffer.
    apply_post_processing(world, active_camera);
}
// ================================================================ //

// ==================== Initialization helpers ==================== //
bool Renderer::init_bgfx(Window* window) {
    // 1) Hook BGFX up to the native window handle.
    bgfx::PlatformData pd{};
    pd.nwh = window->get_native_handle(); // NOTE: currently Windows-only.
    bgfx::setPlatformData(pd);

    m_width  = window->get_width();
    m_height = window->get_height();

    // 2) Configure BGFX renderer and resolution.
    bgfx::Init init{};
    init.type      = bgfx::RendererType::Direct3D12;
    init.platformData = pd;
    init.vendorId  = BGFX_PCI_ID_NONE;
    init.resolution.width  = static_cast<uint32_t>(m_width);
    init.resolution.height = static_cast<uint32_t>(m_height);
    init.resolution.reset  = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        std::cerr << "[Renderer] ERROR: Failed to initialize BGFX.\n";
        return false;
    }

    // 3) Setup default view parameters (will be overridden per-pass).
    init_default_view(window);

    return true;
}

void Renderer::init_default_view(Window* window) {
    bgfx::setViewClear(
        kSceneViewId,
        BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
        0x303030ff,
        1.0f,
        0
    );
    bgfx::setViewRect(
        kSceneViewId,
        0,
        0,
        static_cast<uint16_t>(window->get_width()),
        static_cast<uint16_t>(window->get_height())
    );
    bgfx::setViewName(kSceneViewId, "Scene");
}

bool Renderer::init_selection_program() {
    // TODO: make this configurable instead of hard-coded path.
    std::string baseDir = (ROOT / "shaders/post/selection").string();
    std::string vsPath  = baseDir + "/vs_selection.bin";
    std::string fsPath  = baseDir + "/fs_selection.bin";

    m_selectionProgram =
        ShaderUtils::load_program(vsPath, fsPath, /*destroyShaders=*/true);

    if (!bgfx::isValid(m_selectionProgram)) {
        std::cerr << "[Renderer] ERROR: Failed to create selection mask program.\n";
        return false;
    }

    std::cout << "[Renderer] Selection program loaded. ID: "
              << m_selectionProgram.idx << "\n";
    return true;
}

void Renderer::init_post_process_chain() {
    // Post-process chain operates at screen resolution.
    m_postChain.init(static_cast<uint16_t>(m_width),
                     static_cast<uint16_t>(m_height));

    auto outline = std::make_unique<OutlineEffect>(
        m_fullscreenQuad,
        static_cast<uint16_t>(m_width),
        static_cast<uint16_t>(m_height)
    );

    // Optional tuning:
    // outline->set_outline_color(1.f, 0.8f, 0.1f);
    // outline->set_outline_thickness(2.0f);

    m_outlineEffect = outline.get();
    m_postChain.add_effect(std::move(outline));

    if (m_outlineEffect) {
        std::cout << "[Renderer] OutlineEffect created.\n";
    } else {
        std::cerr << "[Renderer] ERROR: OutlineEffect is NULL.\n";
    }
}

bool Renderer::init_picking_program() {
    // Adjust path to match your project layout
    std::string baseDir = (ROOT / "shaders/post/picking").string();
    std::string vsPath  = baseDir + "/vs_pick.bin";
    std::string fsPath  = baseDir + "/fs_pick.bin";

    m_pickProgram = ShaderUtils::load_program(vsPath, fsPath, /*destroyShaders=*/true);
    if (!bgfx::isValid(m_pickProgram)) {
        std::cerr << "[Renderer] ERROR: Failed to create picking program.\n";
        return false;
    }

    u_idColor = bgfx::createUniform("u_idColor", bgfx::UniformType::Vec4);
    return true;
}

void Renderer::create_picking_target() {
    uint16_t w = static_cast<uint16_t>(m_width);
    uint16_t h = static_cast<uint16_t>(m_height);

    destroy_picking_target();
    if (w == 0 || h == 0) return;

    // 1) GPU render target (full size) where we render per-entity IDs.
    m_pickColor = bgfx::createTexture2D(
        w, h,
        false, 1,
        bgfx::TextureFormat::BGRA8,
        BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP
    );

    if (!bgfx::isValid(m_pickColor)) {
        std::cerr << "[Renderer] ERROR: Failed to create picking color texture.\n";
        return;
    }

    // 2) CPU readback texture (1x1) that we blit a single pixel into.
    m_pickReadbackTex = bgfx::createTexture2D(
        1, 1,
        false, 1,
        bgfx::TextureFormat::BGRA8,
        BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_BLIT_DST
    );

    if (!bgfx::isValid(m_pickReadbackTex)) {
        std::cerr << "[Renderer] ERROR: Failed to create picking readback texture.\n";
        // Clean up pickColor to avoid leaks
        bgfx::destroy(m_pickColor);
        m_pickColor = BGFX_INVALID_HANDLE;
        return;
    }

    // 3) Framebuffer attached to the GPU RT texture
    bgfx::TextureHandle attachments[] = { m_pickColor };
    m_pickFb = bgfx::createFrameBuffer(
        BX_COUNTOF(attachments),
        attachments,
        false // we destroy textures manually
    );

    if (!bgfx::isValid(m_pickFb)) {
        std::cerr << "[Renderer] ERROR: Failed to create picking framebuffer.\n";
        bgfx::destroy(m_pickColor);
        bgfx::destroy(m_pickReadbackTex);
        m_pickColor       = BGFX_INVALID_HANDLE;
        m_pickReadbackTex = BGFX_INVALID_HANDLE;
        return;
    }

    // 4) CPU buffer for ONE pixel (BGRA8) = 4 bytes
    m_pickBuffer.resize(4);
}

void Renderer::destroy_picking_target() {
    if (bgfx::isValid(m_pickFb)) {
        bgfx::destroy(m_pickFb);
    }
    if (bgfx::isValid(m_pickColor)) {
        bgfx::destroy(m_pickColor);
    }
    if (bgfx::isValid(m_pickReadbackTex)) {
        bgfx::destroy(m_pickReadbackTex);
    }

    m_pickFb          = BGFX_INVALID_HANDLE;
    m_pickColor       = BGFX_INVALID_HANDLE;
    m_pickReadbackTex = BGFX_INVALID_HANDLE;
    m_pickBuffer.clear();
}

// ================================================================ //

// ======================== Per Frame Helpers ===================== //
void Renderer::render_scene(World& world, Entity active_camera) {
   // Use view 0 for main scene into m_sceneFb.
    const bgfx::ViewId view_id = kSceneViewId;

    Mat4 view_matrix = world.get_camera_view_matrix(active_camera);
    Mat4 proj_matrix = world.get_camera_proj_matrix(active_camera);
    uint16_t clear_flags = world.get_camera_clear_flags(active_camera);

    bgfx::setViewFrameBuffer(view_id, m_sceneFb);
    bgfx::setViewClear(view_id, clear_flags, 0x303030ff, 1.0f, 0);
    bgfx::setViewRect(view_id, 0, 0,
                      static_cast<uint16_t>(m_width),
                      static_cast<uint16_t>(m_height));
    bgfx::setViewTransform(view_id, view_matrix.ptr(), proj_matrix.ptr());
    bgfx::touch(view_id); // ensure view is cleared even if no draws.

    // Collect draw calls and instance batches from the world.
    std::vector<DrawItem> drawItems;
    drawItems.reserve(256);

    std::unordered_map<InstanceBatchKey, InstanceBatch, InstanceBatchKeyHash> instanceBatches;

    auto modelPool = world.pool<ModelComponent>();
    if (modelPool) {
        for (auto& [entity, model_comp] : *modelPool) {
            if (!model_comp || !model_comp->mesh || !model_comp->material)
                continue;

            MeshData* mesh     = model_comp->mesh.get();
            Material* material = model_comp->material.get();
            Mat4      transform = world.get_world_transform(entity);

            if (material->supports_instancing()) {
                // Group into instance batch by (mesh, material).
                InstanceBatchKey key{ mesh, material };
                auto& batch = instanceBatches[key];

                if (!batch.mesh) {
                    batch.mesh     = model_comp->mesh;
                    batch.material = model_comp->material;
                }
                batch.transforms.push_back(transform);
            } else {
                // Non-instanced: one draw call per object.
                drawItems.push_back({ mesh, material, transform });
            }
        }
    }

    // Submit non-instanced items.
    for (const auto& item : drawItems) {
        item.material->submit(view_id, *item.mesh, item.transform);
    }

    // Submit instance batches.
    for (auto& [key, batch] : instanceBatches) {
        submit_instanced_batch(view_id, batch);
    }
}

void Renderer::render_selection_mask(World& world, Entity active_camera) {
    if (!bgfx::isValid(m_selectionFb) || !bgfx::isValid(m_selectionProgram)) {
        return;
    }

    const bgfx::ViewId view_id = kSelectionViewId;

    Mat4 view_matrix = world.get_camera_view_matrix(active_camera);
    Mat4 proj_matrix = world.get_camera_proj_matrix(active_camera);

    // Render selected geometry to m_selectionFb.
    bgfx::setViewFrameBuffer(view_id, m_selectionFb);
    bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x00000000, 1.0f, 0);
    bgfx::setViewRect(view_id, 0, 0, static_cast<uint16_t>(m_width), static_cast<uint16_t>(m_height));
    bgfx::setViewTransform(view_id, view_matrix.ptr(), proj_matrix.ptr());
    bgfx::touch(view_id);

    auto modelPool = world.pool<ModelComponent>();
    if (!modelPool) {
        return;
    }

    for (auto& [entity, model_comp] : *modelPool) {
        if (!model_comp || !model_comp->mesh)
            continue;

        // Game/editor decides which entities are 'selected'.
        if (!world.is_entity_selected(entity))
            continue;

        MeshData* mesh = model_comp->mesh.get();
        Mat4 transform = world.get_world_transform(entity);

        bgfx::setTransform(transform.ptr());
        bgfx::setVertexBuffer(0, mesh->vbh);
        bgfx::setIndexBuffer(mesh->ibh);

        uint64_t state =
            BGFX_STATE_WRITE_RGB |
            BGFX_STATE_WRITE_A   |
            BGFX_STATE_WRITE_Z   |
            BGFX_STATE_DEPTH_TEST_LESS |
            BGFX_STATE_CULL_CW;

        bgfx::setState(state);
        bgfx::submit(view_id, m_selectionProgram);
    }
}

void Renderer::apply_post_processing(World& world, Entity /*active_camera*/) {
    const bool hasEffect  = (m_outlineEffect != nullptr);
    const bool hasTexture = bgfx::isValid(m_sceneColor);

    if (hasEffect && hasTexture) {
        // Build post-process context for OutlineEffect.
        PostProcessContext ctx;
        ctx.width         = static_cast<uint16_t>(m_width);
        ctx.height        = static_cast<uint16_t>(m_height);
        ctx.nextViewId    = kPostProcessViewBase;
        ctx.sceneDepth    = m_sceneDepth;
        ctx.selectionMask = m_selectionTex;
        ctx.world         = &world;

        // OutlineEffect will usually:
        //  - sample m_sceneColor as input color
        //  - sample sceneDepth for edge detection
        //  - sample selectionMask to know where to draw outlines
        //  - output to backbuffer or another RT.
        m_outlineEffect->apply(ctx, m_sceneColor, BGFX_INVALID_HANDLE);
    } else {
        // Fallback: clear a view to blue so it's obvious we skipped rendering.
        std::cerr << "[Renderer] !!! SKIPPING POST-PROCESS RENDER !!!\n";
        std::cerr << "   > OutlineEffect: " << (m_outlineEffect ? "VALID" : "NULL") << "\n";
        std::cerr << "   > Scene texture handle: " << m_sceneColor.idx << "\n";

        bgfx::setViewFrameBuffer(kPostProcessViewBase, BGFX_INVALID_HANDLE);
        bgfx::setViewRect(kPostProcessViewBase, 0, 0,
                          static_cast<uint16_t>(m_width),
                          static_cast<uint16_t>(m_height));
        bgfx::setViewClear(
            kPostProcessViewBase,
            BGFX_CLEAR_COLOR,
            0x0000FFFF, // blue
            1.0f, 0
        );
        bgfx::touch(kPostProcessViewBase);
    }
}

void Renderer::render_picking(World& world, Entity active_camera) {
      if (!world.m_pickRequest.requested) {
        return;
    }

    if (!bgfx::isValid(m_pickFb) || !bgfx::isValid(m_pickProgram) ||
        !bgfx::isValid(m_pickColor) || !bgfx::isValid(m_pickReadbackTex)) {
        world.m_pickRequest.requested = false;
        return;
    }

    // Capture request state
    m_pickState.active = true;
    m_pickState.mouseX = world.m_pickRequest.mouseX;
    m_pickState.mouseY = world.m_pickRequest.mouseY;
    world.m_pickRequest.requested = false;

    auto clampi = [](int v, int lo, int hi) {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    };

    int mx = clampi(m_pickState.mouseX, 0, m_width  - 1);
    int my = clampi(m_pickState.mouseY, 0, m_height - 1);

    // Optional: if picks are vertically flipped, uncomment:
    // my = (m_height - 1) - my;

    // --- VIEW 3: render entity IDs into full-size pick RT ---
    Mat4 view = world.get_camera_view_matrix(active_camera);
    Mat4 proj = world.get_camera_proj_matrix(active_camera);

    bgfx::setViewFrameBuffer(kPickingViewId, m_pickFb);
    bgfx::setViewRect(kPickingViewId, 0, 0,
                      static_cast<uint16_t>(m_width),
                      static_cast<uint16_t>(m_height));
    bgfx::setViewClear(kPickingViewId,
                       BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                       0x00000000, 1.0f, 0);
    bgfx::setViewTransform(kPickingViewId, view.ptr(), proj.ptr());
    bgfx::touch(kPickingViewId);

    auto modelPool = world.pool<ModelComponent>();
    if (modelPool) {
        for (auto& [entity, model_comp] : *modelPool) {
            if (!model_comp || !model_comp->mesh) continue;

            MeshData* mesh = model_comp->mesh.get();
            Mat4 transform = world.get_world_transform(entity);

            uint32_t id = static_cast<uint32_t>(entity);
            if (id == 0) continue;

            float color[4];
            encode_id_rgba8(id, color);

            bgfx::setTransform(transform.ptr());
            bgfx::setVertexBuffer(0, mesh->vbh);
            bgfx::setIndexBuffer(mesh->ibh);
            bgfx::setUniform(u_idColor, color);

            uint64_t state =
                BGFX_STATE_WRITE_RGB |
                BGFX_STATE_WRITE_A   |
                BGFX_STATE_WRITE_Z   |
                BGFX_STATE_DEPTH_TEST_LESS |
                BGFX_STATE_CULL_CW;

            bgfx::setState(state);
            bgfx::submit(kPickingViewId, m_pickProgram);
        }
    }

    // --- VIEW 4: blit ONLY 1x1 pixel into the 1x1 readback texture ---
    const uint16_t sx = static_cast<uint16_t>(mx);
    const uint16_t sy = static_cast<uint16_t>(my);

    bgfx::blit(
        kPickingBlitViewId,
        m_pickReadbackTex, (uint16_t)0, (uint16_t)0,
        m_pickColor,       sx,          sy,
        (uint16_t)1,       (uint16_t)1
    );

    // Schedule readback
    if (m_pickBuffer.size() >= 4) {
        m_pickState.pendingReadback = true;
        m_pickState.readyFrame = bgfx::readTexture(m_pickReadbackTex, m_pickBuffer.data());
    } else {
        m_pickState.pendingReadback = false;
        m_pickState.active = false;
    }
}

// ================================================================ //

// ======================= Instancing Helpers ===================== //
void Renderer::submit_instanced_batch(bgfx::ViewId viewId, InstanceBatch &batch) {
    if (!batch.mesh || !batch.material) {
        return;
    }

    const uint16_t stride = sizeof(float) * 16; // 4x4 matrix per instance
    const uint32_t totalInstances =
        static_cast<uint32_t>(batch.transforms.size());

    if (totalInstances == 0) {
        return;
    }

    uint32_t offset = 0;
    while (offset < totalInstances) {
        const uint32_t remaining = totalInstances - offset;
        const uint32_t desired   =
            (remaining > kMaxInstancesPerDraw)
                ? kMaxInstancesPerDraw
                : remaining;

        // Check how many instances we can fit in BGFX's transient buffer.
        const uint32_t avail =
            bgfx::getAvailInstanceDataBuffer(desired, stride);
        if (avail == 0) {
            // Out of transient space for this frame.
            // Optional: log a warning or break.
            break;
        }

        const uint32_t count = avail;

        bgfx::InstanceDataBuffer idb;
        bgfx::allocInstanceDataBuffer(&idb, count, stride);

        // Copy transforms into instance buffer.
        uint8_t* dst = idb.data;
        for (uint32_t i = 0; i < count; ++i) {
            const Mat4& m = batch.transforms[offset + i];
            std::memcpy(dst + i * stride, m.ptr(), stride);
        }

        // Let the Material handle actual BGFX submission (shaders, state, etc.).
        batch.material->submit_instanced(
            viewId,
            *batch.mesh,
            &idb,
            count
        );

        offset += count;
    }
}
// ================================================================ //

// ====================== Render Target Helpers =================== //
void Renderer::create_scene_targets() {
    uint16_t w = static_cast<uint16_t>(m_width);
    uint16_t h = static_cast<uint16_t>(m_height);

    // Make it safe to recreate (e.g., on resize).
    destroy_scene_targets();

    std::cout << "[Renderer] Creating scene render targets. Size: "
              << w << "x" << h << "\n";

    if (w == 0 || h == 0) {
        std::cerr << "[Renderer] WARNING: Window size is 0; "
                  << "render targets may be invalid.\n";
    }

    // Color texture.
    m_sceneColor = bgfx::createTexture2D(
        w, h,
        false, 1,
        bgfx::TextureFormat::BGRA8,
        BGFX_TEXTURE_RT
    );

    if (bgfx::isValid(m_sceneColor)) {
        std::cout << "[Renderer] Scene color texture created. Handle: "
                  << m_sceneColor.idx << "\n";
    } else {
        std::cerr << "[Renderer] ERROR: Failed to create scene color texture.\n";
    }

    // Depth texture.
    m_sceneDepth = bgfx::createTexture2D(
        w, h,
        false, 1,
        bgfx::TextureFormat::D24S8,
        BGFX_TEXTURE_RT_WRITE_ONLY
    );

    // Framebuffer with color + depth attachments.
    bgfx::TextureHandle attachments[2] = { m_sceneColor, m_sceneDepth };
    m_sceneFb = bgfx::createFrameBuffer(
        2,
        attachments,
        true // destroy textures when FB is destroyed
    );
}

void Renderer::destroy_scene_targets() {
    if (bgfx::isValid(m_sceneFb)) {
        bgfx::destroy(m_sceneFb);
    }

    m_sceneFb    = BGFX_INVALID_HANDLE;
    m_sceneColor = BGFX_INVALID_HANDLE;
    m_sceneDepth = BGFX_INVALID_HANDLE;
}

void Renderer::create_selection_target() {
    uint16_t w = static_cast<uint16_t>(m_width);
    uint16_t h = static_cast<uint16_t>(m_height);

    // Safe to recreate.
    destroy_selection_target();

    m_selectionTex = bgfx::createTexture2D(
        w, h,
        false, 1,
        bgfx::TextureFormat::BGRA8,
        BGFX_TEXTURE_RT
    );

    bgfx::TextureHandle attachments[1] = { m_selectionTex };
    m_selectionFb = bgfx::createFrameBuffer(
        1,
        attachments,
        true // destroy textures when FB is destroyed
    );
}

void Renderer::destroy_selection_target() {
    if (bgfx::isValid(m_selectionFb)) {
        bgfx::destroy(m_selectionFb);
    }

    m_selectionFb  = BGFX_INVALID_HANDLE;
    m_selectionTex = BGFX_INVALID_HANDLE;
}
// ================================================================ //


