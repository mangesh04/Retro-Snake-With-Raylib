#version 330

// ---------------------------------------------------------------------------
// roundedMask.fs
//
// Minimal post-process pass used ONLY by Theme::NORMAL. It does exactly one
// thing: clips the rendered window to a rounded-rectangle shape. No bloom,
// no scanlines, no phosphor tint, no barrel curvature, no grain — all of
// that lives in composite.fs and is Retro-only.
//
// Mirrors the forceOpaque behavior from composite.fs so Normal-theme
// Pomodoro also gets a solid (non-transparent) rounded panel instead of a
// halo where alpha would otherwise show the desktop through the corners.
// ---------------------------------------------------------------------------

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // raylib's default scene sampler
uniform vec2 resolution;      // window size in pixels
uniform float cornerRadius;   // outer corner radius, px
uniform int forceOpaque;      // 1 = Pomodoro (opaque rounded panel), 0 = buddy (keep source alpha)

// Signed distance from point p to the edge of a centered rounded box of
// half-size b and corner radius r. Negative = inside.
float roundedBoxSDF(vec2 p, vec2 b, float r)
{
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main()
{
    vec4 srcColor = texture(texture0, fragTexCoord);

    vec2 pixelPos = fragTexCoord * resolution;
    vec2 center = resolution * 0.5;
    vec2 halfSize = resolution * 0.5;

    float dist = roundedBoxSDF(pixelPos - center, halfSize, cornerRadius);

    // Antialiased edge, ~1px soft
    float maskAlpha = 1.0 - smoothstep(-1.0, 1.0, dist);

    float outAlpha = (forceOpaque == 1) ? maskAlpha : (srcColor.a * maskAlpha);

    finalColor = vec4(srcColor.rgb, outAlpha);
}
