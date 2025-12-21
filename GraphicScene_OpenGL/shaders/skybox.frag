#version 410 core

in vec2 texCoords;
out vec4 color;

uniform sampler2D dayTexture;
uniform sampler2D nightTexture;
uniform float blendFactor; // 0.0 = night, 1.0 = day

void main()
{
    vec4 dayColor = texture(dayTexture, texCoords);
    vec4 nightColor = texture(nightTexture, texCoords);
    color = mix(nightColor, dayColor, blendFactor);
}