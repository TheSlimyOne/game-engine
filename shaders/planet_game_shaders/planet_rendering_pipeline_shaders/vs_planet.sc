$input a_position, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_color0

#include <bgfx_shader.sh>

uniform vec4 u_time;

float hash1(float n)
{
    return fract(sin(n) * 43758.5453123);
}

void main()
{
    mat4 mtx = mtxFromRows(i_data0, i_data1, i_data2, i_data3);

    vec4 wp = mul(mtx, vec4(a_position.xyz, 1.0));
    gl_Position = mul(u_modelViewProj, wp);

    v_color0 = vec4(1.0,1.0,1.0,1.0);
}