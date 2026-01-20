#include <bgfx_compute.sh>

struct AtomicCounterEntry {
    uint primitiveCountFull;
    uint primitiveCountCulled;
    uint primitiveCountRendered;
    uint padding;
};

struct IndicesBlock {
    uint readIndex;
    uint writeIndex;
    uint deleteIndex;
    uint padding;
};

struct CommandBuffer
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

BUFFER_RW(atomicCounter, AtomicCounterEntry, 0u);
BUFFER_RW(indices, IndicesBlock, 1u);
IMAGE2D_RW(u_read_list, rgba32ui, 2u);

BUFFER_RO(instance_data, vec4, 3u);
BUFFER_WO(u_draw_command_buffer, uvec4, 4u);
BUFFER_WO(u_draw_instance_buffer, vec4, 5u);
BUFFER_RW(u_draw_indirect_count_buffer, int, 6u);

#define indices indices[0]
#define u_draw_indirect_count u_draw_indirect_count_buffer[0]

NUM_THREADS(64u, 1u, 1u)
void main()
{

    uint invocationID = gl_GlobalInvocationID.x;

    if (invocationID == 0u)
    {
        u_draw_indirect_count = 10u;

        for (uint i = 0u; i < 10u; ++i)
        {
            float f = float(i) * 10.0;

            u_draw_instance_buffer[4 * i + 0] = vec4(1.0, 0.0, 0.0, f);   // tx in .w
            u_draw_instance_buffer[4 * i + 1] = vec4(0.0, 1.0, 0.0, 0.0); // ty in .w
            u_draw_instance_buffer[4 * i + 2] = vec4(0.0, 0.0, 1.0, f);   // tz in .w
            u_draw_instance_buffer[4 * i + 3] = vec4(0.0, 0.0, 0.0, 1.0);

            // This writes an indirect command AND sets instanceOffset = i
            drawIndexedIndirect(
                u_draw_command_buffer,
                i,      // command index
                36u,    // numIndices
                1u,     // numInstances
                0u,     // startIndex
                0u,     // startVertex
                i       // instanceOffset (THIS is what you were missing)
            );

        }
    }


    /*
    if (invocationID >= atomicCounter[indices.readIndex].primitiveCountFull)
        return;

    #if BGFX_SHADER_LANGUAGE_HLSL
        uint index;
        InterlockedAdd(u_draw_indirect_count_buffer, 1u, index);
    #else
        uint index = atomicAdd(u_draw_indirect_count_buffer, 1u);
    #endif

    u_draw_instance_buffer[index * 4 + 0] = vec4(1, 0, 0, 0);
    u_draw_instance_buffer[index * 4 + 1] = vec4(0, 1, 0, 0);
    u_draw_instance_buffer[index * 4 + 2] = vec4(0, 0, 1, 0);
    u_draw_instance_buffer[index * 4 + 3] = vec4(0, 0, 0, 1);

    drawIndexedIndirect(
        // Target location params:
        u_draw_command_buffer,	// target buffer
        index,				    // index in buffer

        // Draw call params:
        instance_data[0].w, // number of indices for this draw call
        1u, 					// number of instances for this draw call. You can disable this draw call by setting to zero
        instance_data[0].z, // offset in the index buffer
        instance_data[0].x, // offset in the vertex buffer. Note that you can use this to "reindex" submeshses - all indicies in this draw will be decremented by this amount
        index					// offset in the instance buffer. If you are drawing more than 1 instance per call see gpudrivenrendering for how to handle
    );
    */

}