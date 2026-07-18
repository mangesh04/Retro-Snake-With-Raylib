#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;    // the rendered scene (sceneRT)
uniform sampler2D bloomTex;    // blurred bright-pass result
uniform vec2      resolution;  // window size, px
uniform float     time;
uniform vec3      phosphorColor;  // e.g. (0.25, 1.0, 0.35) for green phosphor
uniform float     bezelThickness; // px
uniform float     cornerRadius;   // px
uniform float     curvature;      // 0 = flat, higher = more barrel bulge

// When 1.0: skip the opaque black bezel frame entirely and use the scene's
// own alpha instead — for windows like the buddy that should stay
// transparent wherever nothing was drawn. When 0.0 (default): behave like a
// real CRT bezel, opaque black frame regardless of what's under it.
uniform float respectSourceAlpha;

// Optional colored accent ring drawn right along the outer rounded edge.
// borderColor.a = 0 disables it entirely.
uniform vec4  borderColor;
uniform float borderThickness;  // px, how wide the visible ring is

out vec4 finalColor;

// Signed distance to a rounded rectangle, centered at origin, half-size b, corner radius r
float roundedBoxSDF(vec2 p, vec2 b, float r)
{
    vec2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

float rand(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec2 uv = fragTexCoord;

    // ---- 1) Barrel curvature: warp UVs outward from center ----------------
    vec2  cc    = uv - 0.5;
    float dist2 = dot(cc, cc);
    vec2  warpedUV = uv + cc * dist2 * curvature;

    // Anything the curvature pushes outside 0..1 is the "tube edge" — solid
    // black for the CRT-bezel look, but transparent when respectSourceAlpha
    // is on (buddy), so a curved buddy window doesn't get black corners either
    if (warpedUV.x < 0.0 || warpedUV.x > 1.0 || warpedUV.y < 0.0 || warpedUV.y > 1.0)
    {
        float edgeAlpha = mix(1.0, 0.0, respectSourceAlpha);
        finalColor = vec4(0.0, 0.0, 0.0, edgeAlpha);
        return;
    }

    vec4 scene = texture(texture0, warpedUV);
    vec4 bloom = texture(bloomTex, warpedUV);

    // ---- 2) Composite scene + bloom -----------------------------------
    vec3 color = scene.rgb + bloom.rgb * 0.9;

    // ---- 3) Phosphor tint ------------------------------------------------
    color *= phosphorColor;

    // ---- 4) Scanlines ------------------------------------------------------
    float scan = sin(warpedUV.y * resolution.y * 3.14159265) * 0.5 + 0.5;
    color *= mix(0.78, 1.0, scan);

    // ---- 5) Aperture-grille / dot mask (subtle RGB column tint) -----------
    float col3 = mod(gl_FragCoord.x, 3.0);
    vec3 mask;
    if (col3 < 1.0)      mask = vec3(1.06, 0.94, 0.94);
    else if (col3 < 2.0) mask = vec3(0.94, 1.06, 0.94);
    else                 mask = vec3(0.94, 0.94, 1.06);
    color *= mask;

    // ---- 6) Grain --------------------------------------------------------
    float grain = (rand(warpedUV * resolution + time) - 0.5) * 0.05;
    color += grain;

    // ---- 7) Vignette --------------------------------------------------------
    float vig = 1.0 - dist2 * 0.6;
    color *= clamp(vig, 0.0, 1.0);

    // ---- 8) Rounded bezel frame --------------------------------------
    vec2 px        = warpedUV * resolution;
    vec2 center    = resolution * 0.5;
    vec2 halfSize  = resolution * 0.5;

    // Inner edge of the bezel band — inside this, the picture shows through
    float innerD    = roundedBoxSDF(px - center, halfSize - bezelThickness, cornerRadius);
    float bezelMask = smoothstep(0.0, 1.5, -innerD);

    // Outer rounded-corner cutoff, so the whole window (bezel included)
    // ends in a rounded rect rather than a hard square
    float outerD    = roundedBoxSDF(px - center, halfSize, cornerRadius);
    float outerMask = smoothstep(0.0, 1.5, -outerD);

    // ---- Two alpha behaviours ------------------------------------------
    // "Opaque bezel" (TV look): frame band is forced fully opaque black,
    // regardless of what the source scene had there.
    float opaqueBezelAlpha = bezelMask * outerMask;

    // "True transparency": alpha comes from the scene's own alpha (so empty
    // space around a sprite — e.g. the buddy — stays see-through), only
    // clipped by the outer rounded corner.
    float sceneAlpha       = max(scene.a, bloom.a);
    float trueAlpha        = sceneAlpha * outerMask;

    float alpha = mix(opaqueBezelAlpha, trueAlpha, respectSourceAlpha);

    // Only darken color to black in the frame band for the opaque-bezel
    // look — in true-transparency mode the sprite's real colors stay put,
    // the bezel band just isn't drawn at all (alpha handles that above).
    color = mix(color * bezelMask, color, respectSourceAlpha);

    // ---- 9) Colored accent border ring, right along the outer edge --------
    float edgeDist   = abs(outerD);
    float borderBand = smoothstep(borderThickness, 0.0, edgeDist) * borderColor.a;
    color = mix(color, borderColor.rgb, borderBand);
    alpha = max(alpha, borderBand);

    finalColor = vec4(color, alpha);
}
