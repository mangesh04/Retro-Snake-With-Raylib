#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Continuous real-time clock — deliberately independent of the sprite's own
// idle/blink frame-swap timers, so this animates smoothly regardless of
// which sheet or how fast it's currently stepping.
uniform float time;

// (u0, v0, u1, v1) — texcoord bounds of the CURRENT frame within its sheet.
// Keeps the wave/glitch UV offsets from sampling into neighbouring frames.
uniform vec4 frameBounds;

out vec4 finalColor;

float rand(float n)
{
    return fract(sin(n) * 43758.5453123);
}

void main()
{
    vec2 uv = fragTexCoord;

    // Normalized position within just this frame (0..1), used to drive
    // effects relative to the sprite itself rather than the whole sheet.
    float localY = (uv.y - frameBounds.y) / max(0.0001, frameBounds.w - frameBounds.y);

    // --- Wavy warp: horizontal bands skew sideways, the wave crest
    // drifting from top to bottom over time ---
    float waveFreq  = 50.0;
    float waveSpeed = 1.4;
    float waveAmp   = 0.001;
    float wave = sin(localY * waveFreq - time * waveSpeed) * waveAmp;
    uv.x += wave;

    // --- Occasional glitch burst: for a short window every few seconds,
    // slice the frame into bands, jitter them sideways, and split channels ---
    float glitchPeriod = 4.5;
    float cycle     = mod(time, glitchPeriod);
    bool  glitching  = cycle < 0.15;

    vec4 texel;
    if (glitching)
    {
        float band   = floor(localY * 18.0);
        float seed   = band + floor(time * 40.0);
        float jitter = (rand(seed) - 0.5) * 0.05;
        uv.x += jitter;

        float splitAmt = 0.008;
        vec2 uvR = clamp(uv + vec2(splitAmt, 0.0), frameBounds.xy, frameBounds.zw);
        vec2 uvB = clamp(uv - vec2(splitAmt, 0.0), frameBounds.xy, frameBounds.zw);
        vec2 uvC = clamp(uv,                        frameBounds.xy, frameBounds.zw);

        float r = texture(texture0, uvR).r;
        float g = texture(texture0, uvC).g;
        float b = texture(texture0, uvB).b;
        float a = texture(texture0, uvC).a;
        texel = vec4(r, g, b, a);
    }
    else
    {
        uv = clamp(uv, frameBounds.xy, frameBounds.zw);
        texel = texture(texture0, uv);
    }

    texel = texel * colDiffuse * fragColor;

    // --- Posterize: crush the palette down for a chunky retro look ---
    float levels = 5.0;
    texel.rgb = floor(texel.rgb * levels + 0.5) / levels;

    // --- Soft vignette for an old-CRT edge falloff ---
    vec2 centered = uv - vec2((frameBounds.x + frameBounds.z) * 0.5,
                               (frameBounds.y + frameBounds.w) * 0.5);
    centered /= vec2(frameBounds.z - frameBounds.x, frameBounds.w - frameBounds.y);
    float vig = smoothstep(0.75, 0.25, dot(centered, centered) * 4.0);
    texel.rgb *= mix(0.7, 1.0, vig);

    finalColor = texel;
}
