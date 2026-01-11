
$input a_position

#include <bgfx_shader.sh>

void main()
{
    // a_position comes from varying.def.sc via $input.
    // u_modelViewProj is a bgfx built-in uniform from bgfx_shader.sh.
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}
