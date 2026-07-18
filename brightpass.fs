#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float threshold;   // set from main.cpp, initPostProcess() — default 0.45

out vec4 finalColor;

void main()
{
    vec4 col = texture(texture0, fragTexCoord);

    // Perceptual luminance
    float brightness = dot(col.rgb, vec3(0.299, 0.587, 0.114));

    // Soft-knee cutoff instead of a hard step, so bloom fades in rather
    // than popping on/off as pixels cross the threshold
    float contrib = smoothstep(threshold, threshold + 0.15, brightness);

    finalColor = vec4(col.rgb * contrib, col.a * contrib);
}
