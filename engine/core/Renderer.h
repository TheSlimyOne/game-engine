// Renderer.h
#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include <cstdint>
#include <vector>
#include <memory>

#include <bgfx/bgfx.h>

#include "Types.h"
#include "FullscreenQuad.h"
#include <post_processing/PostProcessChain.h>

class Window;
class World;
class MeshData;
class Material;
class OutlineEffect;

// A single non-instanced draw call.
struct DrawItem {
    MeshData* mesh   = nullptr;
    Material* material = nullptr;
    Mat4      transform;
};

// Key used to group instances by (mesh, material)
struct InstanceBatchKey {
    MeshData* mesh;
    Material* material;

    bool operator==(const InstanceBatchKey& other) const noexcept {
        return mesh == other.mesh && material == other.material;
    }
};

// Hash for InstanceBatchKey so it can be used in unordered_map
struct InstanceBatchKeyHash {
    std::size_t operator()(const InstanceBatchKey& key) const noexcept {
        return std::hash<void*>()(key.mesh) ^ (std::hash<void*>()(key.material) << 1);
    }
};

// A batch of instances of the same (mesh, material) pair.
struct InstanceBatch {
    std::shared_ptr<MeshData>   mesh;
    std::shared_ptr<Material>   material;
    std::vector<Mat4>           transforms;
};

struct PickState {
    bool     active          = false;  // a pick for which we rendered a pass
    int      mouseX          = 0;
    int      mouseY          = 0;
    bool     pendingReadback = false;
    uint32_t readyFrame      = 0;
};


class Renderer {
public:
    Renderer();
    ~Renderer();

    // ======================== Renderer Interface ==================== //
    // Initialize BGFX and all render targets / post-process effects.
    bool init(Window* window);

    // Clean up all resources.
    void shutdown();

    // Returns time delta (seconds) since last call.
    float get_delta_time();

    // Optional hooks around per-frame rendering.
    void begin_frame();
    void end_frame(World& world);

    // Main entry point to render one frame.
    void draw_frame(World& world);
    // ================================================================ //

private:
    // ==================== Initialization Helpers ==================== //
    bool init_bgfx(Window* window);
    void init_default_view(Window* window);
    bool init_selection_program();
    void init_post_process_chain();
    bool init_picking_program();
    void create_picking_target();
    void destroy_picking_target();
    // ================================================================ //

    // ======================== Per Frame Helpers ===================== //
    void render_scene(World& world, Entity active_camera);
    void render_selection_mask(World& world, Entity active_camera);
    void apply_post_processing(World& world, Entity active_camera);
    void render_picking(World& world, Entity active_camera);
    // ================================================================ //

    // ======================= Instancing Helpers ===================== //
    void submit_instanced_batch(bgfx::ViewId viewId, InstanceBatch& batch);
    // ================================================================ //

    // ====================== Render Target Helpers =================== //
    void create_scene_targets();
    void destroy_scene_targets();
    void create_selection_target();
    void destroy_selection_target();
    // ================================================================ //

private:
    bool    m_initialized = false;
    int32_t m_width       = 0;
    int32_t m_height      = 0;
    double  m_last_dt     = 0.0;

    PickState              m_pickState;
    std::vector<uint8_t>   m_pickBuffer; // width * height * 4 bytes

    // Off-screen scene color + depth.
    bgfx::FrameBufferHandle m_sceneFb    = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle     m_sceneColor = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle     m_sceneDepth = BGFX_INVALID_HANDLE;

    // Selection mask (off-screen).
    bgfx::FrameBufferHandle m_selectionFb      = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle     m_selectionTex     = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle     m_selectionProgram = BGFX_INVALID_HANDLE;

    // Post-processing.
    FullscreenQuad   m_fullscreenQuad;
    PostProcessChain m_postChain;
    OutlineEffect*   m_outlineEffect = nullptr; // owned by m_postChain

    // Picking RT + shader.
    bgfx::FrameBufferHandle m_pickFb           = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle     m_pickColor        = BGFX_INVALID_HANDLE; // GPU Render Target
    bgfx::TextureHandle     m_pickReadbackTex  = BGFX_INVALID_HANDLE; // CPU Readback Texture (NEW)
    bgfx::ProgramHandle     m_pickProgram      = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle     u_idColor          = BGFX_INVALID_HANDLE;
};

#endif // GAME_RENDERER_H
