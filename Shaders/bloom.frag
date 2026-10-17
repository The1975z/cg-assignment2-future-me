#version 330 core

in vec2 TexCoord;

out vec4 colour;

const int BLOOM_BRIGHT_PASS = 0;
const int BLOOM_BLUR = 1;

uniform int bloomMode;
uniform sampler2D sourceTexture;
uniform vec2 blurDirection;
uniform float threshold;

vec3 brightPart(vec2 uv)
{
    vec3 c = texture(sourceTexture, uv).rgb;
    float brightness = max(max(c.r, c.g), c.b);
    return c * smoothstep(threshold, 1.0, brightness);
}

vec3 brightPass()
{
    vec2 texel = 1.0 / vec2(textureSize(sourceTexture, 0));
    vec3 total = brightPart(TexCoord + texel * vec2(-1.0, -1.0))
               + brightPart(TexCoord + texel * vec2(1.0, -1.0))
               + brightPart(TexCoord + texel * vec2(-1.0, 1.0))
               + brightPart(TexCoord + texel * vec2(1.0, 1.0));
    return total * 0.25;
}

vec3 gaussianBlur()
{
    const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 result = texture(sourceTexture, TexCoord).rgb * weights[0];
    for (int i = 1; i < 5; i++)
    {
        result += texture(sourceTexture, TexCoord + blurDirection * float(i)).rgb * weights[i];
        result += texture(sourceTexture, TexCoord - blurDirection * float(i)).rgb * weights[i];
    }
    return result;
}

void main()
{
    if (bloomMode == BLOOM_BRIGHT_PASS)
    {
        colour = vec4(brightPass(), 1.0);
    }
    else if (bloomMode == BLOOM_BLUR)
    {
        colour = vec4(gaussianBlur(), 1.0);
    }
    else
    {
        colour = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
