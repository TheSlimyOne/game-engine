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

BUFFER_RW(atomicCounter, AtomicCounterEntry, 0u);
BUFFER_RW(indices, IndicesBlock, 1u);
BUFFER_RW(indirectBuffer, uvec4, 2u);
BUFFER_RW(u_draw_indirect_count_buffer, int, 3u);

#define indices indices[0]
#define u_draw_indirect_count_buffer u_draw_indirect_count_buffer[0]

NUM_THREADS(1u, 1u, 1u)
void main()
{
    {
        IndicesBlock temp = {
            (indices.readIndex   + 1) % 3,
            (indices.writeIndex  + 1) % 3,
            (indices.deleteIndex + 1) % 3,
            0
        };
        indices = temp;
    }


    uint fullCount   = atomicCounter[indices.readIndex].primitiveCountFull;
    uint culledCount = atomicCounter[indices.readIndex].primitiveCountCulled;

    indirectBuffer[0] = uvec4(fullCount / 64 + 1, 1, 1, 0);

    {
        AtomicCounterEntry temp = {0, 0, 0, 0};
        atomicCounter[indices.deleteIndex] = temp;
    }

    u_draw_indirect_count_buffer = 0;

    //imageStore(u_out, ivec2(0, 0), vec4(indices.readIndex, indices.writeIndex, indices.deleteIndex, 0));
}