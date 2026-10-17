#version 330 core

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;
in vec3 LocalPos;

out vec4 colour;

const int MATERIAL_PLANE = 0;
const int MATERIAL_CLOUD = 1;
const int MATERIAL_MOUNTAIN = 2;
const int MAX_POINT_LIGHTS = 8;
const vec3 CLOUD_SHADOW_TINT = vec3(0.32, 0.36, 0.62);

uniform int materialType;
uniform sampler2D texture_data;
uniform sampler2D shadowMap;
uniform bool useTexture;
uniform vec3 objectColour;
uniform float shininess;
uniform float specularStrength;

uniform vec3 viewPos;
uniform vec3 lightDirection;
uniform vec3 lightColour;
uniform vec3 sunLightColour;
uniform vec3 ambientTopColour;
uniform vec3 ambientBottomColour;

uniform vec3 sunSkyDirection;
uniform vec3 zenithColour;
uniform vec3 midSkyColour;
uniform vec3 horizonSunColour;
uniform vec3 horizonAwayColour;
uniform vec3 belowHorizonColour;
uniform vec3 sunGlowColour;

uniform float fogDensity;
uniform float fogBaseHeight;
uniform float fogHeightFalloff;
uniform float exposure;
uniform vec2 screenSize;

uniform int pointLightCount;
uniform vec3 pointLightPositions[MAX_POINT_LIGHTS];
uniform vec3 pointLightColours[MAX_POINT_LIGHTS];
uniform vec3 cockpitLightColour;

float hash(vec3 p)
{
    p = fract(p * 0.3183099 + vec3(0.71, 0.113, 0.419));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float valueNoise(vec3 p)
{
    vec3 cell = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = mix(hash(cell + vec3(0.0, 0.0, 0.0)), hash(cell + vec3(1.0, 0.0, 0.0)), f.x);
    float b = mix(hash(cell + vec3(0.0, 1.0, 0.0)), hash(cell + vec3(1.0, 1.0, 0.0)), f.x);
    float c = mix(hash(cell + vec3(0.0, 0.0, 1.0)), hash(cell + vec3(1.0, 0.0, 1.0)), f.x);
    float d = mix(hash(cell + vec3(0.0, 1.0, 1.0)), hash(cell + vec3(1.0, 1.0, 1.0)), f.x);
    return mix(mix(a, b, f.y), mix(c, d, f.y), f.z);
}

float fractalNoise(vec3 p)
{
    float total = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++)
    {
        total += valueNoise(p) * amplitude;
        p = p * 2.03 + vec3(1.7, 9.2, 3.4);
        amplitude *= 0.5;
    }
    return total / 0.9375;
}

vec3 skyGradient(vec3 dir)
{
    float sunSide = 0.5;
    if (length(dir.xz) > 0.0001)
    {
        sunSide = dot(normalize(dir.xz), normalize(sunSkyDirection.xz)) * 0.5 + 0.5;
    }
    vec3 horizon = mix(horizonAwayColour, horizonSunColour, pow(sunSide, 2.5));
    if (dir.y >= 0.0)
    {
        float lowBlend = smoothstep(0.0, 0.2, dir.y);
        float highBlend = smoothstep(0.1, 0.7, dir.y);
        return mix(mix(horizon, midSkyColour, lowBlend), zenithColour, highBlend);
    }
    else
    {
        return mix(horizon, belowHorizonColour, smoothstep(0.0, 0.06, -dir.y));
    }
}

vec3 sunGlow(vec3 dir)
{
    float sunAmount = max(dot(dir, sunSkyDirection), 0.0);
    return sunGlowColour * (pow(sunAmount, 12.0) * 0.25 + pow(sunAmount, 64.0) * 0.5 + pow(sunAmount, 600.0) * 2.0);
}

vec3 skyColour(vec3 direction)
{
    vec3 dir = normalize(direction);
    return skyGradient(dir) + sunGlow(dir);
}

float vignette()
{
    vec2 offset = gl_FragCoord.xy / screenSize - 0.5;
    return 1.0 - dot(offset, offset) * 0.7;
}

vec3 toneMap(vec3 c)
{
    c = max(c * exposure * vignette(), vec3(0.0));
    c = (c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14);
    c = mix(vec3(dot(c, vec3(0.2126, 0.7152, 0.0722))), c, 1.15);
    return pow(clamp(c, 0.0, 1.0), vec3(1.0 / 2.2));
}

float ShadowCalculation(vec3 normal, int kernelRadius, float spacing)
{
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0)
    {
        return 0.0;
    }
    else if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
    {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.0004 * (1.0 - dot(normal, lightDirection)), 0.00005);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;
    float samples = 0.0;
    for (int x = -kernelRadius; x <= kernelRadius; ++x)
    {
        for (int y = -kernelRadius; y <= kernelRadius; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize * spacing).r;
            if (currentDepth - bias > pcfDepth)
            {
                shadow += 1.0;
            }
            samples += 1.0;
        }
    }
    return shadow / samples;
}

vec3 ambientLight(vec3 normal)
{
    return mix(ambientBottomColour, ambientTopColour, normal.y * 0.5 + 0.5);
}

vec3 diffuseLight(vec3 normal, vec3 lightDir, vec3 lightTint, float wrap)
{
    float diff = max((dot(normal, lightDir) + wrap) / (1.0 + wrap), 0.0);
    return lightTint * diff;
}

vec3 specularLight(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 lightTint)
{
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    return lightTint * spec * specularStrength;
}

vec3 pointLight(int index, vec3 normal, vec3 viewDir)
{
    vec3 toLight = pointLightPositions[index] - FragPos;
    float lightDistance = length(toLight);
    vec3 lightDir = toLight / lightDistance;
    float attenuation = 1.0 / (1.0 + 0.7 * lightDistance + 1.8 * lightDistance * lightDistance);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    return pointLightColours[index] * (diff + spec) * attenuation;
}

vec3 cockpitGlow(vec3 albedo)
{
    if (LocalPos.z < 28.5 || LocalPos.z > 33.2 || LocalPos.y < 3.0)
    {
        return vec3(0.0);
    }
    float glass = 1.0 - smoothstep(0.1, 0.3, dot(albedo, vec3(0.2126, 0.7152, 0.0722)));
    return cockpitLightColour * glass;
}

vec3 shadePlane(vec3 normal, vec3 viewDir)
{
    vec3 albedo = objectColour;
    if (useTexture)
    {
        albedo = texture(texture_data, TexCoord).rgb;
    }
    float lit = 1.0 - ShadowCalculation(normal, 1, 1.0);
    vec3 lighting = ambientLight(normal) + diffuseLight(normal, lightDirection, lightColour, 0.0) * lit;
    for (int i = 0; i < pointLightCount; i++)
    {
        lighting += pointLight(i, normal, viewDir);
    }
    float facing = max(dot(normal, viewDir), 0.0);
    float fresnel = 0.04 + 0.96 * pow(1.0 - facing, 5.0);
    vec3 reflection = skyColour(reflect(-viewDir, normal)) * fresnel * 0.55;
    float rim = pow(1.0 - facing, 3.0) * max(dot(normal, sunSkyDirection), 0.0);
    vec3 specular = specularLight(normal, viewDir, lightDirection, lightColour) * lit;
    return albedo * lighting + specular + reflection + sunGlowColour * rim * 0.6 + cockpitGlow(albedo);
}

vec3 shadeCloud(vec3 normal, vec3 viewDir)
{
    vec3 detailPosition = FragPos * 0.015;
    float detail = fractalNoise(detailPosition);
    vec3 bumps = vec3(fractalNoise(detailPosition + vec3(3.1, 0.0, 0.0)),
                      fractalNoise(detailPosition + vec3(0.0, 7.7, 0.0)),
                      fractalNoise(detailPosition + vec3(0.0, 0.0, 1.3))) - 0.5;
    float nearFade = 1.0 - smoothstep(600.0, 2500.0, length(viewPos - FragPos));
    vec3 finePosition = FragPos * 0.06;
    vec3 fineBumps = vec3(fractalNoise(finePosition + vec3(11.0, 0.0, 0.0)),
                          fractalNoise(finePosition + vec3(0.0, 23.0, 0.0)),
                          fractalNoise(finePosition + vec3(0.0, 0.0, 37.0))) - 0.5;
    vec3 bumpedNormal = normalize(normal + bumps * 0.8 + fineBumps * 0.5 * nearFade);
    float heightShade = clamp((FragPos.y - fogBaseHeight + 20.0) / 160.0, 0.0, 1.0);
    float fineDetail = mix(1.0, 1.0 + 0.2 * fineBumps.x, nearFade);
    vec3 albedo = objectColour * (0.8 + 0.3 * detail) * fineDetail * mix(0.45, 1.1, heightShade);
    vec3 lighting = ambientLight(bumpedNormal)
                  + diffuseLight(bumpedNormal, sunSkyDirection, sunLightColour, 0.2) * mix(0.5, 1.0, heightShade);
    float backLight = pow(max(dot(-viewDir, sunSkyDirection), 0.0), 4.0);
    float edge = 1.0 - max(dot(normal, viewDir), 0.0);
    float silverLining = (pow(1.0 - max(dot(bumpedNormal, viewDir), 0.0), 3.0) * 0.5 + pow(edge, 6.0) * 1.5) * backLight;
    lighting *= mix(1.0, 0.6, backLight);
    float shadow = ShadowCalculation(normal, 3, 5.0);
    vec3 shaded = albedo * lighting + sunGlowColour * silverLining * 1.2;
    return shaded * mix(vec3(1.0), CLOUD_SHADOW_TINT, shadow);
}

vec3 shadeMountain(vec3 normal, vec3 viewDir)
{
    float detail = fractalNoise(FragPos * 0.006);
    float snowLine = fogBaseHeight + 350.0 + (detail - 0.5) * 400.0;
    float snow = smoothstep(0.45, 0.7, normal.y + (detail - 0.5) * 0.4)
               * smoothstep(snowLine - 80.0, snowLine + 80.0, FragPos.y);
    vec3 rock = vec3(0.07, 0.065, 0.08) * (0.6 + 0.8 * detail);
    vec3 albedo = mix(rock, vec3(0.82, 0.85, 0.92), snow);
    vec3 lighting = ambientLight(normal) * 0.7 + diffuseLight(normal, sunSkyDirection, sunLightColour, 0.05);
    return albedo * lighting + specularLight(normal, viewDir, sunSkyDirection, sunLightColour) * snow * 0.2;
}

vec3 applyFog(vec3 shaded, vec3 viewDir)
{
    float viewDistance = length(viewPos - FragPos);
    float heightAbove = max(FragPos.y - fogBaseHeight, 0.0);
    float density = fogDensity * exp(-heightAbove / fogHeightFalloff);
    float fogAmount = clamp(1.0 - exp(-viewDistance * density), 0.0, 1.0);
    vec3 fogDirection = normalize(-viewDir);
    vec3 fogColour = skyGradient(fogDirection) + sunGlow(fogDirection) * mix(0.3, 1.0, fogAmount * fogAmount);
    return mix(shaded, fogColour, fogAmount);
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 shaded;
    if (materialType == MATERIAL_PLANE)
    {
        shaded = shadePlane(normal, viewDir);
    }
    else if (materialType == MATERIAL_CLOUD)
    {
        shaded = shadeCloud(normal, viewDir);
    }
    else if (materialType == MATERIAL_MOUNTAIN)
    {
        shaded = shadeMountain(normal, viewDir);
    }
    else
    {
        shaded = objectColour;
    }
    colour = vec4(toneMap(applyFog(shaded, viewDir)), 1.0);
}
