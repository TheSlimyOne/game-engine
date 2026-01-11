$input v_worldPos

#include <bgfx_shader.sh>

uniform vec4 u_gridParams;
uniform vec4 u_gridColor;
uniform vec4 u_xAxisPosColor;
uniform vec4 u_xAxisNegColor;
uniform vec4 u_zAxisPosColor;
uniform vec4 u_zAxisNegColor;

void main()
{
    float cellSize      = u_gridParams.x;
    float lineThickness = u_gridParams.y;
    float fadeDist      = u_gridParams.z;
    float axisWidth     = u_gridParams.w;

    vec2 worldXZ = v_worldPos.xz;

    // Work in grid space so grid repeats every `cellSize` world units
    vec2 pos = worldXZ / cellSize;   // grid coordinates, lines at integer values

    // --- 1. GRID LINE MASK (AA’d) ---

    // How big a pixel is in grid-space
    vec2 gridWidth = fwidth(pos) * lineThickness;

    // Distance to nearest integer in each dimension (0 on line, 1 at center of cell)
    vec2 fracPart = fract(pos);
    vec2 gridDist = 2.0 * min(fracPart, 1.0 - fracPart);

    // Normalize by pixel width
    vec2 gridLines = gridDist / gridWidth;

    float lineValue  = min(gridLines.x, gridLines.y);
    float gridFactor = 1.0 - clamp(lineValue, 0.0, 1.0);

    // Completely outside any line
    if (gridFactor <= 0.0) {
        discard;
    }

    // --- 2. AXIS COLORING ---

    vec4 finalColor = u_gridColor;

    // World space axis detection:
    //   x=0 line is Z axis
    //   z=0 line is X axis
    if (abs(worldXZ.x) < axisWidth) {
        // Z-axis (x == 0), color depends on sign of z
        finalColor = mix(u_zAxisNegColor, u_zAxisPosColor, step(0.0, worldXZ.y));
    }
    else if (abs(worldXZ.y) < axisWidth) {
        // X-axis (z == 0), color depends on sign of x
        finalColor = mix(u_xAxisNegColor, u_xAxisPosColor, step(0.0, worldXZ.x));
    }

    // --- 3. DISTANCE FADE (optional) ---

    float dist = length(worldXZ);
    float fade = 1.0;
    if (fadeDist > 0.0) {
        fade = clamp(1.0 - dist / fadeDist, 0.0, 1.0);
    }

    float alpha = gridFactor * fade;

    // Optional: tiny alpha cutoff to avoid super faint fragments
    if (alpha <= 0.01) {
        discard;
    }

    gl_FragColor = vec4(finalColor.rgb, alpha);
}
