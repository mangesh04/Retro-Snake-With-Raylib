#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 blurDir;   // set from main.cpp: (1/w, 0) for the H pass, (0, 1/h) for the V pass

out vec4 finalColor;

void main()
{
    // 5-tap Gaussian-ish weights (center + 2 taps each side, mirrored)
    float weights[5] = float[](
        0.2270270270,
        0.1945945946,
        0.1216216216,
        0.0540540541,
        0.0162162162
    );

    vec4 sum = texture(texture0, fragTexCoord) * weights[0];

    for (int i = 1; i < 5; i++)
    {
        // Spread taps out a bit (x2) for a softer, wider-reaching bloom
        vec2 offset = blurDir * float(i) * 2.0;
        sum += texture(texture0, fragTexCoord + offset) * weights[i];
        sum += texture(texture0, fragTexCoord - offset) * weights[i];
    }

    finalColor = sum;
}
