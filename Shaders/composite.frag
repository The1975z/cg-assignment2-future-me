#version 330 core

in vec2 TexCoord;

out vec4 colour;

uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;
uniform float bloomStrength;

void main()
{
    vec3 scene = texture(sceneTexture, TexCoord).rgb;
    vec3 bloom = texture(bloomTexture, TexCoord).rgb * bloomStrength;
    vec3 result = vec3(1.0) - (vec3(1.0) - scene) * (vec3(1.0) - clamp(bloom, 0.0, 1.0));
    colour = vec4(result, 1.0);
}
