#version 330 core

in vec3 Direction;

out vec4 colour;

uniform vec3 sunSkyDirection;
uniform vec3 zenithColour;
uniform vec3 midSkyColour;
uniform vec3 horizonSunColour;
uniform vec3 horizonAwayColour;
uniform vec3 belowHorizonColour;
uniform vec3 sunGlowColour;
uniform vec3 sunDiscColour;
uniform float exposure;
uniform vec2 screenSize;
uniform bool occlusionPass;

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
    for (int i = 0; i < 5; i++)
    {
        total += valueNoise(p) * amplitude;
        p = p * 2.03 + vec3(1.7, 9.2, 3.4);
        amplitude *= 0.5;
    }
    return total / 0.96875;
}

vec3 skyColour(vec3 direction)
{
    vec3 dir = normalize(direction);
    float sunSide = 0.5;
    if (length(dir.xz) > 0.0001)
    {
        sunSide = dot(normalize(dir.xz), normalize(sunSkyDirection.xz)) * 0.5 + 0.5;
    }
    vec3 horizon = mix(horizonAwayColour, horizonSunColour, pow(sunSide, 2.5));
    vec3 result;
    if (dir.y >= 0.0)
    {
        float lowBlend = smoothstep(0.0, 0.2, dir.y);
        float highBlend = smoothstep(0.1, 0.7, dir.y);
        result = mix(mix(horizon, midSkyColour, lowBlend), zenithColour, highBlend);
    }
    else
    {
        result = mix(horizon, belowHorizonColour, smoothstep(0.0, 0.06, -dir.y));
    }
    float sunAmount = max(dot(dir, sunSkyDirection), 0.0);
    result += sunGlowColour * (pow(sunAmount, 12.0) * 0.25 + pow(sunAmount, 64.0) * 0.5 + pow(sunAmount, 600.0) * 2.0);
    return result;
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

vec3 addStars(vec3 dir, vec3 base)
{
    if (dir.y < 0.08)
    {
        return base;
    }
    vec3 grid = dir * 220.0;
    vec3 cell = floor(grid);
    if (hash(cell) < 0.996)
    {
        return base;
    }
    float starShape = smoothstep(0.3, 0.0, length(fract(grid) - 0.5));
    float darkness = smoothstep(0.15, 0.6, dir.y) * (1.0 - max(dot(dir, sunSkyDirection), 0.0));
    return base + vec3(0.9, 0.95, 1.1) * starShape * darkness * 0.5;
}

vec3 addCirrus(vec3 dir, vec3 base)
{
    if (dir.y < 0.02)
    {
        return base;
    }
    vec2 projected = dir.xz / dir.y;
    vec3 streakPosition = vec3(projected.x * 1.6 + projected.y * 0.9, 4.0, projected.y * 0.45 - projected.x * 0.2);
    float density = smoothstep(0.5, 0.85, fractalNoise(streakPosition * 1.4));
    float fade = smoothstep(0.02, 0.15, dir.y) * (1.0 - smoothstep(0.35, 0.8, dir.y));
    float sunAmount = max(dot(dir, sunSkyDirection), 0.0);
    vec3 litColour = mix(vec3(0.85, 0.42, 0.5), vec3(1.6, 0.85, 0.45), pow(sunAmount, 3.0));
    return mix(base, litColour, density * fade * 0.65);
}

float sunburst(vec3 dir)
{
    vec3 side = normalize(cross(sunSkyDirection, vec3(0.0, 1.0, 0.0)));
    vec3 upward = cross(side, sunSkyDirection);
    vec3 offset = dir - sunSkyDirection * dot(dir, sunSkyDirection);
    float angle = atan(dot(offset, upward), dot(offset, side));
    vec2 circle = vec2(cos(angle), sin(angle));
    float broad = valueNoise(vec3(circle * 4.0, 1.7));
    float fine = valueNoise(vec3(circle * 11.0, 5.3));
    return clamp(0.25 + broad * fine * 1.6, 0.0, 1.0);
}

float sunLightSource(vec3 dir)
{
    float sunAmount = max(dot(dir, sunSkyDirection), 0.0);
    float disc = smoothstep(0.99985, 0.99995, sunAmount);
    float halo = pow(sunAmount, 600.0) * 0.5 * sunburst(dir);
    return disc + halo;
}

void main()
{
    vec3 dir = normalize(Direction);
    if (occlusionPass)
    {
        colour = vec4(sunLightSource(dir), 0.0, 0.0, 1.0);
    }
    else
    {
        vec3 result = skyColour(dir);
        result = addStars(dir, result);
        result = addCirrus(dir, result);
        float disc = smoothstep(0.99990, 0.99995, dot(dir, sunSkyDirection));
        result += sunDiscColour * disc;
        colour = vec4(toneMap(result), 1.0);
    }
}
