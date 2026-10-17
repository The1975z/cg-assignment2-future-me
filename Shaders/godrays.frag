#version 330 core

in vec2 TexCoord;

out vec4 colour;

const int SAMPLE_COUNT = 96;

uniform sampler2D occlusionMap;
uniform vec2 sunScreenPosition;
uniform vec3 rayColour;
uniform float rayExposure;
uniform float decay;
uniform float density;
uniform float weight;

void main()
{
    vec2 marchStep = (TexCoord - sunScreenPosition) * density / float(SAMPLE_COUNT);
    vec2 sampleCoord = TexCoord;
    float illumination = 1.0;
    float rays = 0.0;
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        sampleCoord -= marchStep;
        rays += texture(occlusionMap, sampleCoord).r * illumination * weight;
        illumination *= decay;
    }
    float foreground = texture(occlusionMap, TexCoord).g;
    colour = vec4(rayColour * rays * rayExposure * (1.0 - foreground * 0.9), 1.0);
}
