#version 330 core

in vec2 Offset;

out vec4 colour;

uniform vec3 glowColour;
uniform float intensity;
uniform float coreSize;

void main()
{
    float radiusSquared = dot(Offset, Offset);
    if (radiusSquared > 1.0)
    {
        discard;
    }
    float edge = 1.0 - radiusSquared;
    float halo = exp(-radiusSquared * 9.0) * edge;
    float core = exp(-radiusSquared / max(coreSize, 0.0001)) * edge;
    vec3 result = glowColour * halo * 0.6 + mix(glowColour, vec3(1.0), 0.6) * core;
    colour = vec4(result * intensity, 1.0);
}
