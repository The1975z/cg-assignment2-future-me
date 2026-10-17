#version 330 core

in vec2 Offset;
in float Density;
in float Brightness;

out vec4 colour;

uniform vec3 smokeColour;
uniform vec3 smokeShadowColour;
uniform float alphaCutoff;

void main()
{
    float radiusSquared = dot(Offset, Offset);
    if (radiusSquared > 1.0)
    {
        discard;
    }
    float alpha = exp(-radiusSquared * 3.0) * (1.0 - radiusSquared) * Density;
    if (alpha < alphaCutoff)
    {
        discard;
    }
    float sunSide = clamp(Offset.x * 0.55 - Offset.y * 0.15 + 0.5, 0.0, 1.0);
    vec3 shaded = mix(smokeShadowColour, smokeColour, sunSide) * Brightness;
    colour = vec4(shaded, alpha);
}
