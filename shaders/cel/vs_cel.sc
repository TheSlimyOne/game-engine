$input a_position, a_normal, a_texcoord0
$output v_worldPos, v_normal, v_texcoord0

#include <bgfx_shader.sh>

void main()
{
    // Transform position
    vec3 worldPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;
    v_worldPos = worldPos;

    // Transform normal
    v_normal = mul((mat3)u_model[0], a_normal);

    // Pass UVs
    v_texcoord0 = a_texcoord0;

    gl_Position = mul(u_viewProj, vec4(worldPos, 1.0));
}