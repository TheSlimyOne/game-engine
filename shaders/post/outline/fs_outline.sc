$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_sceneColor, 0);
SAMPLER2D(s_mask,       1);

uniform vec4 u_texelSize;        // (1/width, 1/height, 0, 0)
uniform vec4 u_outlineColor;     // rgb = color
uniform vec4 u_outlineThickness; // x = thickness in pixels

void main()
{
    vec2 uv = v_texcoord0;

    vec4 sceneCol = texture2D(s_sceneColor, uv);
    float center  = texture2D(s_mask, uv).r;

    // Use the uniform thickness
    vec2 texel = u_texelSize.xy * u_outlineThickness.x;

    // Simple 4-tap neighbor sample
    float m0 = texture2D(s_mask, uv + vec2( texel.x, 0.0)).r;
    float m1 = texture2D(s_mask, uv + vec2(-texel.x, 0.0)).r;
    float m2 = texture2D(s_mask, uv + vec2(0.0,  texel.y)).r;
    float m3 = texture2D(s_mask, uv + vec2(0.0, -texel.y)).r;

    float maxN = max(max(m0, m1), max(m2, m3));

    // Edge logic: neighbor has mask (1) but center is empty (0)
    // using step(0.5, x) makes it strictly 0 or 1
    float edge = step(0.5, maxN) * (1.0 - step(0.5, center));

    vec3 outlineRgb = u_outlineColor.rgb;
    vec3 finalRgb   = mix(sceneCol.rgb, outlineRgb, edge);

    // FORCE Alpha to 1.0 to prevent accidental transparency
    gl_FragColor = vec4(finalRgb, 1.0);
}