#include <bgfx_compute.sh>

IMAGE2D_RW(u_counter, r32ui, 0);

NUM_THREADS(8, 8, 1)

void main()
{
     imageAtomicAdd(u_counter, ivec2(0, 0), uvec4(2, 0, 0, 0));
}
