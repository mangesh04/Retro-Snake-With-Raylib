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
    finalColor = vec4(0.0, 0.0, 0.0, 0.0);
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

float outerD    = roundedBoxSDF(px - center, halfSize, cornerRadius);
float outerMask = smoothstep(0.0, 1.5, -outerD);

// Alpha always comes from what was actually drawn — NOT from bloom. Bloom's
// blur used to spread alpha past the sprite's real edge, which is what
// showed up as a soft background/halo around the buddy. Clamping to
// scene.a fixes that, and it's also what removes Pomodoro's old forced
// black bezel band: Pomodoro already paints its own opaque background, so
// it stays opaque here with no separate frame needed.
float alpha = scene.a * outerMask;


    // ---- 9) Colored accent border ring, right along the outer edge --------
    // Flat-topped ring instead of a single-pixel peak: solid for most of
    // borderThickness, with a ~0.75px feather on the outer and inner edges.
    // A peaked band left most of the ring as a partial blend, which let the
    // scanline pass show through as a dashed/dotted line.
    float outerFeather = smoothstep(0.75, -0.75, outerD);
    float innerFeather = smoothstep(-(borderThickness - 0.75), -(borderThickness + 0.75), outerD);
    float borderBand   = clamp(outerFeather - innerFeather, 0.0, 1.0) * borderColor.a;
    color = mix(color, borderColor.rgb, borderBand);
    alpha = max(alpha, borderBand);

    finalColor = vec4(color, alpha);
}