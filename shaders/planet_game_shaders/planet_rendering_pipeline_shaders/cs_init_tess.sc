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
BUFFER_RW(exec_indirect_buffer, uvec4, 2u);

uniform vec4 u_params;
#define u_startingCount u_params.x
#define exec_indirect_buffer exec_indirect_buffer[0]


#define indices indices[0]

NUM_THREADS(1u, 1u, 1u)
void main()
{
    {
        AtomicCounterEntry temp = {u_startingCount, 0, 0, 0};
        atomicCounter[0] = temp;
    }

    {
        IndicesBlock temp = { 2, 0, 1, 0 };
        indices = temp;
    }

    exec_indirect_buffer = uvec4(u_startingCount / 64 + 1, 1, 1, 0);


    //imageStore(u_out, ivec2(0, 0), vec4(indices));
}