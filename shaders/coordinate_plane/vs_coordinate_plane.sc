$input a_position
$output v_worldPos

#include <bgfx_shader.sh>

uniform vec4 u_gridPlane;

void main()
{
    float planeScale = u_gridPlane.x;

    // Scale the plane in object space
    vec3 scaled_position = a_position * planeScale;

    // Standard bgfx pattern
    vec4 worldPos = mul(u_model[0], vec4(scaled_position, 1.0));

    v_worldPos = worldPos.xyz;

    gl_Position = mul(u_viewProj, worldPos);
}
