#version 330 core

in vec2 TexCoord;

out vec4 colour;

const int OVERLAY_WINDOW = 1;
const int OVERLAY_COCKPIT = 2;

uniform int overlayMode;
uniform vec2 screenSize;
uniform float bankAngle;

float roundedBox(vec2 p, vec2 halfSize, float radius)
{
    vec2 q = abs(p) - halfSize + vec2(radius);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - radius;
}

float segmentDistance(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

vec4 passengerWindow(vec2 p)
{
    float glass = roundedBox(p, vec2(0.26, 0.35), 0.2);
    float bezel = roundedBox(p, vec2(0.33, 0.43), 0.25);
    if (glass < 0.0)
    {
        float edgeShadow = smoothstep(-0.05, 0.0, glass);
        return vec4(0.02, 0.02, 0.03, edgeShadow * 0.55 + 0.04);
    }
    else if (bezel < 0.0)
    {
        float depth = glass / (glass - bezel);
        vec3 bezelColour = mix(vec3(0.3, 0.29, 0.28), vec3(0.74, 0.72, 0.69), depth);
        return vec4(bezelColour, 1.0);
    }
    else
    {
        float light = 0.78 + 0.14 * p.y - 0.18 * length(p * vec2(0.8, 1.0));
        return vec4(vec3(0.88, 0.85, 0.8) * light, 1.0);
    }
}

vec3 attitudeIndicator(vec2 local)
{
    float horizonSide = -local.x * sin(bankAngle) + local.y * cos(bankAngle);
    vec3 face = mix(vec3(0.45, 0.27, 0.1), vec3(0.12, 0.38, 0.85), step(0.0, horizonSide));
    face = mix(face, vec3(1.0), 1.0 - smoothstep(0.02, 0.05, abs(horizonSide)));
    float wingBar = step(abs(local.y), 0.05) * step(0.25, abs(local.x)) * step(abs(local.x), 0.65);
    float centreDot = 1.0 - step(0.06, length(local));
    return mix(face, vec3(1.0, 0.85, 0.1), max(wingBar, centreDot)) * 0.9;
}

vec3 navigationDisplay(vec2 local)
{
    vec3 screen = vec3(0.01, 0.015, 0.02);
    float arc = abs(length(local + vec2(0.0, 0.9)) - 1.4);
    float compass = (1.0 - smoothstep(0.015, 0.035, arc)) * step(-0.6, local.y);
    float route = (1.0 - smoothstep(0.015, 0.035, abs(local.x + local.y * 0.35 + 0.26))) * step(-0.75, local.y);
    float aircraft = 1.0 - step(0.08, length(local - vec2(0.0, -0.75)));
    screen = mix(screen, vec3(0.9), compass);
    screen = mix(screen, vec3(0.9, 0.25, 0.9), route);
    return mix(screen, vec3(1.0, 0.9, 0.2), aircraft);
}

vec4 flightDisplays(vec2 p)
{
    vec2 displaySize = vec2(0.075, 0.058);
    vec2 primaryCentre = vec2(-0.26, -0.36);
    vec2 navigationCentre = vec2(-0.06, -0.36);
    if (roundedBox(p - primaryCentre, displaySize, 0.008) < 0.0)
    {
        return vec4(attitudeIndicator((p - primaryCentre) / displaySize.y), 1.0);
    }
    else if (roundedBox(p - navigationCentre, displaySize, 0.008) < 0.0)
    {
        return vec4(navigationDisplay((p - navigationCentre) / displaySize.y), 1.0);
    }
    else
    {
        return vec4(0.0);
    }
}

vec4 cockpitFrame(vec2 p)
{
    float dashboardTop = -0.24 + 0.05 * cos(p.x * 2.4);
    if (p.y < dashboardTop)
    {
        vec3 panel = vec3(0.05, 0.052, 0.06) * (1.0 + (p.y - dashboardTop) * 1.5);
        panel += vec3(0.07) * (1.0 - smoothstep(0.0, 0.012, dashboardTop - p.y));
        vec4 display = flightDisplays(p);
        return vec4(mix(panel, display.rgb, display.a), 1.0);
    }

    float centrePost = segmentDistance(p, vec2(0.3, dashboardTop), vec2(0.42, 0.6));
    float sidePost = segmentDistance(p, vec2(-0.6, dashboardTop), vec2(-0.5, 0.6));
    float overhead = 0.37 + 0.04 * cos(p.x * 2.8);
    if (min(centrePost, sidePost) < 0.035 || p.y > overhead)
    {
        return vec4(0.03, 0.03, 0.035, 1.0);
    }
    else
    {
        return vec4(0.6, 0.7, 0.8, 0.03);
    }
}

void main()
{
    vec2 p = (TexCoord - 0.5) * vec2(screenSize.x / screenSize.y, 1.0);
    if (overlayMode == OVERLAY_WINDOW)
    {
        colour = passengerWindow(p);
    }
    else if (overlayMode == OVERLAY_COCKPIT)
    {
        colour = cockpitFrame(p);
    }
    else
    {
        colour = vec4(0.0);
    }
}
