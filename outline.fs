#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

// (u0, v0, u1, v1) — texcoord bounds of the CURRENT frame within its sheet.
// Same convention as retro.fs — keeps sampling from bleeding into
// neighbouring frames when the sprite sheet has more than one frame.
uniform vec4 frameBounds;

// rgb = glow color, a = current pulse intensity (set from the CPU side,
// breathing over time while the buddy is focused).
uniform vec4 glowColor;

out vec4 finalColor;

void main()
{
    vec2 uv = clamp(fragTexCoord, frameBounds.xy, frameBounds.zw);
    float a = texture(texture0, uv).a;

    // Anywhere the sprite has meaningful opacity, output a flat glow color
    // instead of the sprite's real pixels. This gets stamped several times
    // at small offsets around the sprite's real position (see
    // drawDesktopBuddy) to build a soft glowing outline that follows the
    // sprite's actual silhouette rather than a generic circular halo.
    float mask = step(0.08, a);
    finalColor = vec4(glowColor.rgb, mask * glowColor.a);
}
