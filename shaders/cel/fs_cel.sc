$input v_worldPos, v_normal, v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_lightDir; 
uniform vec4 u_camPos;
uniform vec4 u_baseColor;
uniform vec4 u_shadowColor;
uniform vec4 u_toonParams;
uniform vec4 u_texConfig;

SAMPLER2D(s_texColor, 0); 

void main()
{
    // 1. Base Color
    vec4 texColor = vec4(1.0, 1.0, 1.0, 1.0);
    if (u_texConfig.x > 0.5) {
        texColor = texture2D(s_texColor, v_texcoord0);
    }

    vec3 N = normalize(v_normal);
    vec3 L = normalize(u_lightDir.xyz);
    vec3 V = normalize(u_camPos.xyz - v_worldPos);

    // 2. Lighting Ramp
    float NdotL = dot(N, L);
    float lightIntensity = NdotL * 0.5 + 0.5; 
    float shadowThreshold = u_toonParams.x;
    float shadowSmooth = u_toonParams.y;
    float ramp = smoothstep(shadowThreshold - shadowSmooth, shadowThreshold + shadowSmooth, lightIntensity);

    // 3. COLOR MIXING (Fixed "Too White" issue)
    
    // Hack: Multiply by 0.85. Real objects absorb light. 
    // If we return 1.0 here, adding specular later forces it to clip to white.
    vec3 litColor = texColor.rgb * u_baseColor.rgb * 0.85; 
    
    vec3 shadowState = texColor.rgb * u_shadowColor.rgb; // Shadow is already darkened by u_shadowColor
    vec3 finalColor = mix(shadowState, litColor, ramp);

    // 4. Rim Light (Significantly Reduced)
    float NdotV = dot(N, V);
    float rimDot = 1.0 - clamp(NdotV, 0.0, 1.0);
    // Multiply by 0.5 to keep it subtle
    float rimIntensity = smoothstep(u_toonParams.w, 1.0, rimDot) * u_toonParams.z * ramp * 0.5;
    vec3 rimColor = vec3(1.0, 1.0, 1.0) * rimIntensity;

    // 5. Specular (Sharpened and Reduced)
    vec3 H = normalize(L + V);
    float NdotH = dot(N, H);
    float specIntensity = smoothstep(0.985, 0.99, NdotH); // Sharper dot
    
    // Multiplied by 0.3. Previously it was likely too high.
    vec3 specColor = vec3(1.0, 1.0, 1.0) * specIntensity * 0.3 * ramp;

    // 6. Output
    gl_FragColor = vec4(finalColor + rimColor + specColor, u_baseColor.a * texColor.a);
}