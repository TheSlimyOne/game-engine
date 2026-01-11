$input a_position, i_data0, i_data1, i_data2, i_data3
$output v_color0

#include <bgfx_shader.sh>

void main()
{
    // Reconstruct model/world matrix from per-instance columns.
    // This expects i_dataN to be the columns of your Mat4.
    mat4 model = mtxFromCols(i_data0, i_data1, i_data2, i_data3);

    // Standard bgfx pattern: viewProj * (model * position)
    vec4 worldPos = mul(model, vec4(a_position, 1.0));
    gl_Position   = mul(u_viewProj, worldPos);

    v_color0 = vec4(0.741, 0.627, 0.31, 1.0);
}
