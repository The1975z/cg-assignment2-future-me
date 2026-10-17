#define _CRT_SECURE_NO_WARNINGS
#undef GLFW_DLL

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "Libs/Mesh.h"
#include "Libs/Shader.h"
#include "Libs/Window.h"
#include "ProjectPaths.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_WINDOWS_UTF8
#include <stb_image_write.h>

struct SkySettings
{
    glm::vec3 sunDirection;
    glm::vec3 lightDirection;
    glm::vec3 lightColour;
    glm::vec3 sunLightColour;
    glm::vec3 zenithColour;
    glm::vec3 midSkyColour;
    glm::vec3 horizonSunColour;
    glm::vec3 horizonAwayColour;
    glm::vec3 belowHorizonColour;
    glm::vec3 sunGlowColour;
    glm::vec3 sunDiscColour;
    glm::vec3 ambientTopColour;
    glm::vec3 ambientBottomColour;
    float fogDensity;
    float fogHeightFalloff;
    float exposure;
};

struct AircraftLight
{
    glm::vec3 localPosition;
    glm::vec3 colour;
    float glowSize;
    int pattern;
};

struct MountainPeak
{
    float x;
    float z;
    float height;
    float radius;
};

struct FlareGhost
{
    float offset;
    float size;
    glm::vec3 colour;
    float intensity;
};

struct ShadowMap
{
    GLuint framebuffer;
    GLuint depthTexture;
    int size;
};

struct RenderTarget
{
    GLuint framebuffer;
    GLuint texture;
    int width;
    int height;
};

struct MultisampleTarget
{
    GLuint framebuffer;
    GLuint colourBuffer;
    GLuint depthBuffer;
    int width;
    int height;
};

struct Camera
{
    glm::vec3 position;
    float yaw;
    float pitch;
};

struct AttachedView
{
    glm::vec3 localEye;
    glm::vec3 localTarget;
    float fieldOfView;
    float nearPlane;
    int overlay;
};

struct ViewSetup
{
    glm::mat4 view;
    glm::vec3 eye;
    float fieldOfView;
    float nearPlane;
    int overlay;
};

struct MouseLook
{
    bool firstMove;
    double lastX;
    double lastY;
};

struct InputState
{
    int viewMode;
    bool captureRequested;
    bool captureHeld;
    bool freeHeld;
    bool resetHeld;
    bool viewHeld[4];
    MouseLook mouse;
};

const GLint WIDTH = 800;
const GLint HEIGHT = 600;
const int SHADOW_SIZE = 4096;
const int OCCLUSION_WIDTH = 400;
const int OCCLUSION_HEIGHT = 300;
const int BLOOM_WIDTH = 400;
const int BLOOM_HEIGHT = 300;
const int SCENE_SAMPLES = 4;

const int BLOOM_BRIGHT_PASS = 0;
const int BLOOM_BLUR = 1;

const int VIEW_PHOTO = 0;
const int VIEW_CHASE = 1;
const int VIEW_WINDOW = 2;
const int VIEW_COCKPIT = 3;
const int VIEW_FREE = 4;

const int OVERLAY_NONE = 0;
const int OVERLAY_WINDOW = 1;
const int OVERLAY_COCKPIT = 2;

const float SUN_AZIMUTH = 15.0f;
const float SUN_ELEVATION = 3.5f;

const int MATERIAL_PLANE = 0;
const int MATERIAL_CLOUD = 1;
const int MATERIAL_MOUNTAIN = 2;

const int LIGHT_STEADY = 0;
const int LIGHT_STROBE = 1;
const int LIGHT_BEACON = 2;

const int MAX_POINT_LIGHTS = 8;

const float CLOUD_DECK_HEIGHT = -260.0f;
const float CAPTURE_TIME = 0.03f;

const std::filesystem::path shaderDirectory = std::filesystem::u8path(OPENGL_STARTER_SHADER_DIR);
const std::filesystem::path textureDirectory = std::filesystem::u8path(OPENGL_STARTER_TEXTURE_DIR);
const std::filesystem::path modelDirectory = std::filesystem::u8path(OPENGL_STARTER_MODEL_DIR);
const std::filesystem::path projectDirectory = shaderDirectory.parent_path();

const std::string captureFileName = "Assignment2_67050651_PAIRID.png";
const std::string titleText = "FUTURE ME";
const std::string sloganText = "BORN TO FLY.";
const float SKYWRITING_LETTER_GAP = 0.24f;

const glm::vec3 photoCameraPosition(0.0f, 0.0f, 0.0f);
const glm::vec3 photoCameraTarget(0.0f, -11.0f, -100.0f);

const glm::vec3 airplanePosition(-4.0f, -12.0f, -118.0f);
const float airplaneYaw = glm::radians(108.0f);
const float airplanePitch = glm::radians(-1.0f);
const float airplaneRoll = glm::radians(14.0f);

const std::vector<AircraftLight> aircraftLights =
{
    { glm::vec3(34.45f, 1.75f, -18.7f), glm::vec3(1.0f, 0.12f, 0.08f), 1.6f, LIGHT_STEADY },
    { glm::vec3(-34.45f, 1.75f, -18.7f), glm::vec3(0.15f, 1.0f, 0.45f), 1.6f, LIGHT_STEADY },
    { glm::vec3(34.5f, 1.8f, -19.3f), glm::vec3(1.0f, 1.0f, 1.0f), 2.8f, LIGHT_STROBE },
    { glm::vec3(-34.5f, 1.8f, -19.3f), glm::vec3(1.0f, 1.0f, 1.0f), 2.8f, LIGHT_STROBE },
    { glm::vec3(0.0f, 5.05f, 0.0f), glm::vec3(1.0f, 0.15f, 0.08f), 1.8f, LIGHT_BEACON },
    { glm::vec3(0.0f, -2.5f, 0.0f), glm::vec3(1.0f, 0.15f, 0.08f), 1.8f, LIGHT_BEACON },
    { glm::vec3(-1.25f, 4.4f, 30.2f), glm::vec3(1.0f, 0.32f, 0.05f), 1.3f, LIGHT_STEADY }
};

const AttachedView chaseView = { glm::vec3(10.0f, 14.0f, -95.0f), glm::vec3(0.0f, 1.0f, 10.0f), 45.0f, 2.0f, OVERLAY_NONE };
const AttachedView windowView = { glm::vec3(3.6f, 0.85f, -9.0f), glm::vec3(30.0f, -2.5f, -8.0f), 60.0f, 0.1f, OVERLAY_WINDOW };
const AttachedView cockpitView = { glm::vec3(0.6f, 4.55f, 32.2f), glm::vec3(22.0f, 2.8f, 80.0f), 65.0f, 0.1f, OVERLAY_COCKPIT };

const std::vector<glm::vec3> engineExhausts =
{
    glm::vec3(-11.7f, -2.1f, -6.3f),
    glm::vec3(-21.2f, -1.8f, -8.4f),
    glm::vec3(11.7f, -2.1f, -6.3f),
    glm::vec3(21.2f, -1.8f, -8.4f)
};

const std::vector<MountainPeak> mountainPeaks =
{
    { -9000.0f, -17000.0f, 1300.0f, 5200.0f },
    { -2500.0f, -23000.0f, 1650.0f, 6400.0f },
    { 12500.0f, -21000.0f, 1100.0f, 5200.0f },
    { -21000.0f, -20000.0f, 1500.0f, 7000.0f },
    { 3000.0f, -14000.0f, 700.0f, 3200.0f }
};

const std::vector<FlareGhost> flareGhosts =
{
    { 0.0f, 0.3f, glm::vec3(1.0f, 0.75f, 0.45f), 0.12f },
    { 0.6f, 0.05f, glm::vec3(0.5f, 0.8f, 1.0f), 0.1f },
    { 1.0f, 0.1f, glm::vec3(1.0f, 0.6f, 0.3f), 0.07f },
    { 1.4f, 0.07f, glm::vec3(0.6f, 1.0f, 0.6f), 0.06f },
    { 1.8f, 0.16f, glm::vec3(0.8f, 0.5f, 1.0f), 0.05f }
};

Mesh* skyDome = nullptr;
Mesh* cloudDeck = nullptr;
Mesh* cumulusClouds = nullptr;
Mesh* mountains = nullptr;
Mesh* airplane = nullptr;
Mesh* glowQuad = nullptr;
Mesh* skywriting = nullptr;
Mesh* contrails = nullptr;

Shader* sceneShader = nullptr;
Shader* skyShader = nullptr;
Shader* glowShader = nullptr;
Shader* depthShader = nullptr;
Shader* smokeShader = nullptr;
Shader* occlusionShader = nullptr;
Shader* godRayShader = nullptr;
Shader* bloomShader = nullptr;
Shader* compositeShader = nullptr;
Shader* overlayShader = nullptr;

GLuint airplaneTexture = 0;
ShadowMap airplaneShadow = { 0, 0, 0 };
RenderTarget sunOcclusion = { 0, 0, 0, 0 };
MultisampleTarget sceneTarget = { 0, 0, 0, 0, 0 };
RenderTarget sceneColour = { 0, 0, 0, 0 };
RenderTarget bloomPing = { 0, 0, 0, 0 };
RenderTarget bloomPong = { 0, 0, 0, 0 };
Camera camera = { glm::vec3(0.0f), -90.0f, 0.0f };
glm::vec3 eyePosition(0.0f);

void SetInt(Shader* shader, const char* name, int value)
{
    glUniform1i(shader->GetUniformLocation(name), value);
}

void SetFloat(Shader* shader, const char* name, float value)
{
    glUniform1f(shader->GetUniformLocation(name), value);
}

void SetVec3(Shader* shader, const char* name, const glm::vec3& value)
{
    glUniform3fv(shader->GetUniformLocation(name), 1, glm::value_ptr(value));
}

void SetMat4(Shader* shader, const char* name, const glm::mat4& value)
{
    glUniformMatrix4fv(shader->GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

float HashNoise(int x, int y, int z)
{
    unsigned int h = static_cast<unsigned int>(x) * 73856093u;
    h ^= static_cast<unsigned int>(y) * 19349663u;
    h ^= static_cast<unsigned int>(z) * 83492791u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return static_cast<float>(h & 0xFFFFFFu) / 16777215.0f;
}

float FadeCurve(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

float ValueNoise(float x, float y, float z)
{
    float floorX = std::floor(x);
    float floorY = std::floor(y);
    float floorZ = std::floor(z);
    int ix = static_cast<int>(floorX);
    int iy = static_cast<int>(floorY);
    int iz = static_cast<int>(floorZ);
    float fx = FadeCurve(x - floorX);
    float fy = FadeCurve(y - floorY);
    float fz = FadeCurve(z - floorZ);

    float a = glm::mix(HashNoise(ix, iy, iz), HashNoise(ix + 1, iy, iz), fx);
    float b = glm::mix(HashNoise(ix, iy + 1, iz), HashNoise(ix + 1, iy + 1, iz), fx);
    float c = glm::mix(HashNoise(ix, iy, iz + 1), HashNoise(ix + 1, iy, iz + 1), fx);
    float d = glm::mix(HashNoise(ix, iy + 1, iz + 1), HashNoise(ix + 1, iy + 1, iz + 1), fx);
    return glm::mix(glm::mix(a, b, fy), glm::mix(c, d, fy), fz);
}

float FractalNoise(float x, float y, float z, int octaves)
{
    float total = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    float range = 0.0f;
    for (int i = 0; i < octaves; i++)
    {
        total += ValueNoise(x * frequency, y * frequency, z * frequency) * amplitude;
        range += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.03f;
    }
    return total / range;
}

float BillowNoise(float x, float y, float z, int octaves)
{
    float total = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    float range = 0.0f;
    for (int i = 0; i < octaves; i++)
    {
        total += std::fabs(ValueNoise(x * frequency, y * frequency, z * frequency) * 2.0f - 1.0f) * amplitude;
        range += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.03f;
    }
    return total / range;
}

glm::vec3 DirectionFromAngles(float azimuthDegrees, float elevationDegrees)
{
    float azimuth = glm::radians(azimuthDegrees);
    float elevation = glm::radians(elevationDegrees);
    return glm::normalize(glm::vec3(std::sin(azimuth) * std::cos(elevation),
                                    std::sin(elevation),
                                    -std::cos(azimuth) * std::cos(elevation)));
}

SkySettings CreateSunriseSky()
{
    SkySettings sky;
    sky.sunDirection = DirectionFromAngles(SUN_AZIMUTH, SUN_ELEVATION);
    sky.lightDirection = glm::normalize(glm::vec3(0.335f, 0.423f, 0.843f));
    sky.lightColour = glm::vec3(1.9f, 1.25f, 0.8f);
    sky.sunLightColour = glm::vec3(2.4f, 0.95f, 0.45f);
    sky.zenithColour = glm::vec3(0.05f, 0.08f, 0.25f);
    sky.midSkyColour = glm::vec3(0.3f, 0.3f, 0.52f);
    sky.horizonSunColour = glm::vec3(1.5f, 0.55f, 0.18f);
    sky.horizonAwayColour = glm::vec3(0.52f, 0.33f, 0.5f);
    sky.belowHorizonColour = glm::vec3(0.4f, 0.28f, 0.36f);
    sky.sunGlowColour = glm::vec3(1.8f, 0.8f, 0.3f);
    sky.sunDiscColour = glm::vec3(60.0f, 45.0f, 25.0f);
    sky.ambientTopColour = glm::vec3(0.14f, 0.17f, 0.32f);
    sky.ambientBottomColour = glm::vec3(0.32f, 0.22f, 0.24f);
    sky.fogDensity = 1.0f / 9000.0f;
    sky.fogHeightFalloff = 300.0f;
    sky.exposure = 0.85f;
    return sky;
}

Mesh* BuildMesh(const std::vector<VertexPNU>& vertices, const std::vector<unsigned int>& indices)
{
    Mesh* mesh = new Mesh();
    mesh->CreateMesh(vertices, indices);
    return mesh;
}

void AppendGridIndices(std::vector<unsigned int>& indices, unsigned int first, int rows, int columns)
{
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            unsigned int a = first + row * (columns + 1) + column;
            unsigned int b = a + columns + 1;
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(a + 1);
            indices.push_back(a + 1);
            indices.push_back(b);
            indices.push_back(b + 1);
        }
    }
}

void RecomputeGridNormals(std::vector<VertexPNU>& vertices, unsigned int first, int stacks, int slices, const glm::vec3& centre)
{
    int columns = slices + 1;
    for (int stack = 1; stack < stacks; stack++)
    {
        for (int slice = 0; slice <= slices; slice++)
        {
            int previousSlice = slice - 1;
            int nextSlice = slice + 1;
            if (previousSlice < 0)
            {
                previousSlice = slices - 1;
            }
            else if (nextSlice > slices)
            {
                nextSlice = 1;
            }

            unsigned int current = first + stack * columns + slice;
            glm::vec3 east = vertices[first + stack * columns + nextSlice].position
                           - vertices[first + stack * columns + previousSlice].position;
            glm::vec3 south = vertices[first + (stack + 1) * columns + slice].position
                            - vertices[first + (stack - 1) * columns + slice].position;
            glm::vec3 normal = glm::cross(east, south);
            if (glm::length(normal) > 0.000001f)
            {
                normal = glm::normalize(normal);
                if (glm::dot(normal, vertices[current].position - centre) < 0.0f)
                {
                    normal = -normal;
                }
                vertices[current].normal = normal;
            }
        }
    }
}

void AppendEllipsoid(std::vector<VertexPNU>& vertices, std::vector<unsigned int>& indices,
                     const glm::vec3& centre, const glm::vec3& radius,
                     int stacks, int slices, float roughness, float seed)
{
    unsigned int first = static_cast<unsigned int>(vertices.size());
    for (int stack = 0; stack <= stacks; stack++)
    {
        float phi = glm::pi<float>() * stack / stacks;
        for (int slice = 0; slice <= slices; slice++)
        {
            float theta = glm::two_pi<float>() * slice / slices;
            glm::vec3 unit(std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta));
            float bump = 1.0f;
            if (roughness > 0.0f)
            {
                bump += roughness * (FractalNoise(unit.x * 2.5f + seed, unit.y * 2.5f, unit.z * 2.5f - seed, 3) - 0.5f);
            }
            VertexPNU vertex{};
            vertex.position = centre + unit * radius * bump;
            vertex.normal = glm::normalize(unit / radius);
            vertex.uv = glm::vec2(static_cast<float>(slice) / slices, static_cast<float>(stack) / stacks);
            vertices.push_back(vertex);
        }
    }
    if (roughness > 0.0f)
    {
        RecomputeGridNormals(vertices, first, stacks, slices, centre);
    }
    AppendGridIndices(indices, first, stacks, slices);
}

void AppendCumulusLump(std::vector<VertexPNU>& vertices, std::vector<unsigned int>& indices,
                       const glm::vec3& centre, float radius, int depth, std::mt19937& generator)
{
    std::uniform_real_distribution<float> random(0.0f, 1.0f);
    float seed = random(generator) * 50.0f;
    glm::vec3 shape(radius, radius * 0.85f, radius);

    if (depth > 0)
    {
        AppendEllipsoid(vertices, indices, centre, shape, 14, 22, 0.4f, seed);
    }
    else
    {
        AppendEllipsoid(vertices, indices, centre, shape, 9, 14, 0.45f, seed);
        return;
    }

    int childCount = 4 + static_cast<int>(random(generator) * 3.0f);
    for (int child = 0; child < childCount; child++)
    {
        float angle = random(generator) * glm::two_pi<float>();
        float elevation = glm::mix(0.15f, 1.25f, random(generator));
        float childRadius = radius * glm::mix(0.35f, 0.55f, random(generator));
        glm::vec3 direction(std::cos(angle) * std::cos(elevation), std::sin(elevation), std::sin(angle) * std::cos(elevation));
        glm::vec3 childCentre = centre + direction * shape * 0.78f;
        AppendCumulusLump(vertices, indices, childCentre, childRadius, depth - 1, generator);
    }
}

Mesh* CreateSkyDome()
{
    std::vector<VertexPNU> vertices;
    std::vector<unsigned int> indices;
    AppendEllipsoid(vertices, indices, glm::vec3(0.0f), glm::vec3(1.0f), 48, 96, 0.0f, 0.0f);
    return BuildMesh(vertices, indices);
}

Mesh* CreateGlowQuad()
{
    std::vector<VertexPNU> vertices(4);
    vertices[0].position = glm::vec3(-1.0f, -1.0f, 0.0f);
    vertices[1].position = glm::vec3(1.0f, -1.0f, 0.0f);
    vertices[2].position = glm::vec3(1.0f, 1.0f, 0.0f);
    vertices[3].position = glm::vec3(-1.0f, 1.0f, 0.0f);
    vertices[0].uv = glm::vec2(0.0f, 0.0f);
    vertices[1].uv = glm::vec2(1.0f, 0.0f);
    vertices[2].uv = glm::vec2(1.0f, 1.0f);
    vertices[3].uv = glm::vec2(0.0f, 1.0f);
    for (VertexPNU& vertex : vertices)
    {
        vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    std::vector<unsigned int> indices = { 0, 1, 2, 0, 2, 3 };
    return BuildMesh(vertices, indices);
}

float CloudDeckHeight(float x, float z)
{
    float distanceFromCamera = glm::length(glm::vec2(x - photoCameraPosition.x, z - photoCameraPosition.z));
    float puffFade = glm::smoothstep(4500.0f, 1500.0f, distanceFromCamera);
    float mound = glm::smoothstep(0.35f, 0.75f, FractalNoise(x * 0.00035f, 1.7f, z * 0.00035f, 4));
    float billow = BillowNoise(x * 0.0018f, 5.3f, z * 0.0018f, 4);
    float puff = BillowNoise(x * 0.008f, 9.1f, z * 0.008f, 3);
    return CLOUD_DECK_HEIGHT + mound * 110.0f + billow * 70.0f + puff * 14.0f * puffFade;
}

glm::vec3 CloudDeckNormal(float x, float z, float step)
{
    float left = CloudDeckHeight(x - step, z);
    float right = CloudDeckHeight(x + step, z);
    float back = CloudDeckHeight(x, z - step);
    float front = CloudDeckHeight(x, z + step);
    return glm::normalize(glm::vec3(left - right, 2.0f * step, back - front));
}

Mesh* CreateCloudDeck()
{
    const int rings = 220;
    const int segments = 400;
    const float innerRadius = 15.0f;
    const float outerRadius = 32000.0f;

    std::vector<VertexPNU> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(rings * (segments + 1));

    for (int ring = 0; ring < rings; ring++)
    {
        float t = static_cast<float>(ring) / (rings - 1);
        float radius = innerRadius * std::pow(outerRadius / innerRadius, t);
        float step = std::max(radius * 0.02f, 4.0f);
        for (int segment = 0; segment <= segments; segment++)
        {
            float angle = glm::two_pi<float>() * segment / segments;
            float x = photoCameraPosition.x + radius * std::cos(angle);
            float z = photoCameraPosition.z + radius * std::sin(angle);
            VertexPNU vertex{};
            vertex.position = glm::vec3(x, CloudDeckHeight(x, z), z);
            vertex.normal = CloudDeckNormal(x, z, step);
            vertex.uv = glm::vec2(x, z) * 0.001f;
            vertices.push_back(vertex);
        }
    }
    AppendGridIndices(indices, 0, rings - 1, segments);
    return BuildMesh(vertices, indices);
}

void AppendCloudTower(std::vector<VertexPNU>& vertices, std::vector<unsigned int>& indices,
                      const glm::vec3& base, float size, float height, std::mt19937& generator)
{
    std::uniform_real_distribution<float> random(0.0f, 1.0f);
    float baseSeed = random(generator) * 50.0f;
    AppendEllipsoid(vertices, indices, base + glm::vec3(0.0f, size * 0.15f, 0.0f),
                    glm::vec3(size * 1.4f, size * 0.5f, size * 1.2f), 16, 28, 0.35f, baseSeed);

    int lumpCount = 6 + static_cast<int>(random(generator) * 5.0f);
    for (int lump = 0; lump < lumpCount; lump++)
    {
        float angle = random(generator) * glm::two_pi<float>();
        float sizeRoll = random(generator);
        float seed = random(generator) * 50.0f;

        float rise = static_cast<float>(lump + 1) / lumpCount;
        float spread = size * 0.55f * (1.0f - rise * 0.6f);
        float radius = size * (0.7f - rise * 0.3f) * (0.8f + sizeRoll * 0.4f);
        glm::vec3 centre = base + glm::vec3(std::cos(angle) * spread, height * rise * 0.85f, std::sin(angle) * spread);
        std::mt19937 detailGenerator(static_cast<unsigned int>(seed * 1000.0f));
        AppendCumulusLump(vertices, indices, centre, radius, 1, detailGenerator);
    }
}

void AppendSunCloud(std::vector<VertexPNU>& vertices, std::vector<unsigned int>& indices, std::mt19937& generator)
{
    const float cloudDistance = 8000.0f;
    const int lumpCount = 40;
    std::uniform_real_distribution<float> random(0.0f, 1.0f);

    glm::vec3 sunDirection = DirectionFromAngles(SUN_AZIMUTH, SUN_ELEVATION);
    glm::vec3 across = glm::normalize(glm::cross(sunDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 along = glm::normalize(glm::vec3(sunDirection.x, 0.0f, sunDirection.z));
    glm::vec3 sunPoint = photoCameraPosition + sunDirection * cloudDistance;
    float deckHeight = CloudDeckHeight(sunPoint.x, sunPoint.z);

    for (int lump = 0; lump < lumpCount; lump++)
    {
        float acrossRoll = random(generator) - 0.5f;
        float alongRoll = random(generator) - 0.5f;
        float heightRoll = random(generator) - 0.5f;
        float sizeRoll = random(generator);
        float seed = random(generator) * 50.0f;

        float rise = static_cast<float>(lump) / (lumpCount - 1);
        float spread = glm::mix(620.0f, 300.0f, rise);
        float radius = glm::mix(210.0f, 80.0f, rise) * (0.75f + 0.5f * sizeRoll);
        float height = glm::mix(deckHeight + 60.0f, sunPoint.y + 5.0f, rise) + heightRoll * 60.0f;
        glm::vec3 centre = glm::vec3(sunPoint.x, height, sunPoint.z) + across * (acrossRoll * spread) + along * (alongRoll * 200.0f);
        std::mt19937 detailGenerator(static_cast<unsigned int>(seed * 1000.0f));
        AppendCumulusLump(vertices, indices, centre, radius, 1, detailGenerator);
    }
}

Mesh* CreateCumulusClouds()
{
    std::vector<VertexPNU> vertices;
    std::vector<unsigned int> indices;
    std::mt19937 generator(67050651);
    std::uniform_real_distribution<float> random(0.0f, 1.0f);

    for (int tower = 0; tower < 10; tower++)
    {
        float distanceRoll = random(generator);
        float angleRoll = random(generator);
        float sizeRoll = random(generator);
        float heightRoll = random(generator);

        float towerDistance = 6000.0f + distanceRoll * 8000.0f;
        float towerAngle = glm::radians((angleRoll - 0.5f) * 70.0f);
        float x = photoCameraPosition.x + std::sin(towerAngle) * towerDistance;
        float z = photoCameraPosition.z - std::cos(towerAngle) * towerDistance;
        float size = 140.0f + sizeRoll * 160.0f;
        float height = size * (1.2f + heightRoll * 1.6f);

        glm::vec3 base(x, CloudDeckHeight(x, z) - size * 0.2f, z);
        AppendCloudTower(vertices, indices, base, size, height, generator);
    }
    AppendSunCloud(vertices, indices, generator);
    return BuildMesh(vertices, indices);
}

float MountainHeight(const MountainPeak& peak, float x, float z)
{
    float dx = x - peak.x;
    float dz = z - peak.z;
    float distance = std::sqrt(dx * dx + dz * dz) / peak.radius;
    float base = CLOUD_DECK_HEIGHT - 300.0f;
    if (distance >= 1.0f)
    {
        return base;
    }
    float shape = std::pow(1.0f - distance, 1.7f);
    float ridge = 1.0f - std::fabs(FractalNoise(x * 0.0009f, 21.0f, z * 0.0009f, 5) * 2.0f - 1.0f);
    float rough = FractalNoise(x * 0.004f, 47.0f, z * 0.004f, 3);
    return base + peak.height * shape * (0.55f + 0.6f * ridge) + rough * 60.0f * shape;
}

glm::vec3 MountainNormal(const MountainPeak& peak, float x, float z, float step)
{
    float left = MountainHeight(peak, x - step, z);
    float right = MountainHeight(peak, x + step, z);
    float back = MountainHeight(peak, x, z - step);
    float front = MountainHeight(peak, x, z + step);
    return glm::normalize(glm::vec3(left - right, 2.0f * step, back - front));
}

void AppendMountain(std::vector<VertexPNU>& vertices, std::vector<unsigned int>& indices, const MountainPeak& peak)
{
    const int rings = 56;
    const int segments = 160;
    unsigned int first = static_cast<unsigned int>(vertices.size());
    float step = std::max(peak.radius / rings, 10.0f);

    for (int ring = 0; ring <= rings; ring++)
    {
        float radius = peak.radius * static_cast<float>(ring) / rings;
        for (int segment = 0; segment <= segments; segment++)
        {
            float angle = glm::two_pi<float>() * segment / segments;
            float x = peak.x + radius * std::cos(angle);
            float z = peak.z + radius * std::sin(angle);
            VertexPNU vertex{};
            vertex.position = glm::vec3(x, MountainHeight(peak, x, z), z);
            vertex.normal = MountainNormal(peak, x, z, step);
            vertex.uv = glm::vec2(0.0f);
            vertices.push_back(vertex);
        }
    }
    AppendGridIndices(indices, first, rings, segments);
}

Mesh* CreateMountains()
{
    std::vector<VertexPNU> vertices;
    std::vector<unsigned int> indices;
    for (const MountainPeak& peak : mountainPeaks)
    {
        AppendMountain(vertices, indices, peak);
    }
    return BuildMesh(vertices, indices);
}

std::vector<glm::vec2> EllipseStroke(const glm::vec2& centre, const glm::vec2& radius, int segments)
{
    std::vector<glm::vec2> points;
    for (int segment = 0; segment <= segments; segment++)
    {
        float angle = glm::two_pi<float>() * segment / segments;
        points.push_back(centre + glm::vec2(std::sin(angle) * radius.x, std::cos(angle) * radius.y));
    }
    return points;
}

std::vector<std::vector<glm::vec2>> LetterStrokes(char letter)
{
    if (letter == 'F')
    {
        return {
            { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(0.55f, 1.0f) },
            { glm::vec2(0.0f, 0.52f), glm::vec2(0.42f, 0.52f) }
        };
    }
    else if (letter == 'U')
    {
        return {
            { glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 0.32f), glm::vec2(0.03f, 0.15f), glm::vec2(0.11f, 0.04f),
              glm::vec2(0.275f, 0.0f), glm::vec2(0.44f, 0.04f), glm::vec2(0.52f, 0.15f), glm::vec2(0.55f, 0.32f),
              glm::vec2(0.55f, 1.0f) }
        };
    }
    else if (letter == 'T')
    {
        return {
            { glm::vec2(0.0f, 1.0f), glm::vec2(0.6f, 1.0f) },
            { glm::vec2(0.3f, 1.0f), glm::vec2(0.3f, 0.0f) }
        };
    }
    else if (letter == 'R')
    {
        return {
            { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(0.36f, 1.0f), glm::vec2(0.48f, 0.96f),
              glm::vec2(0.55f, 0.87f), glm::vec2(0.57f, 0.75f), glm::vec2(0.55f, 0.63f), glm::vec2(0.48f, 0.54f),
              glm::vec2(0.36f, 0.5f), glm::vec2(0.0f, 0.5f) },
            { glm::vec2(0.3f, 0.5f), glm::vec2(0.57f, 0.0f) }
        };
    }
    else if (letter == 'E')
    {
        return {
            { glm::vec2(0.55f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.55f, 0.0f) },
            { glm::vec2(0.0f, 0.52f), glm::vec2(0.45f, 0.52f) }
        };
    }
    else if (letter == 'M')
    {
        return {
            { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(0.4f, 0.32f), glm::vec2(0.8f, 1.0f),
              glm::vec2(0.8f, 0.0f) }
        };
    }
    else if (letter == 'B')
    {
        return {
            { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(0.32f, 1.0f), glm::vec2(0.44f, 0.96f),
              glm::vec2(0.5f, 0.88f), glm::vec2(0.51f, 0.79f), glm::vec2(0.48f, 0.68f), glm::vec2(0.4f, 0.57f),
              glm::vec2(0.3f, 0.53f), glm::vec2(0.0f, 0.53f) },
            { glm::vec2(0.3f, 0.53f), glm::vec2(0.44f, 0.5f), glm::vec2(0.53f, 0.41f), glm::vec2(0.56f, 0.28f),
              glm::vec2(0.53f, 0.14f), glm::vec2(0.44f, 0.04f), glm::vec2(0.32f, 0.0f), glm::vec2(0.0f, 0.0f) }
        };
    }
    else if (letter == 'O')
    {
        return { EllipseStroke(glm::vec2(0.31f, 0.5f), glm::vec2(0.31f, 0.5f), 24) };
    }
    else if (letter == 'N')
    {
        return {
            { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(0.6f, 0.0f), glm::vec2(0.6f, 1.0f) }
        };
    }
    else if (letter == 'L')
    {
        return {
            { glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.5f, 0.0f) }
        };
    }
    else if (letter == 'Y')
    {
        return {
            { glm::vec2(0.0f, 1.0f), glm::vec2(0.3f, 0.5f), glm::vec2(0.6f, 1.0f) },
            { glm::vec2(0.3f, 0.5f), glm::vec2(0.3f, 0.0f) }
        };
    }
    else if (letter == '.')
    {
        return {
            { glm::vec2(0.04f, 0.03f), glm::vec2(0.08f, 0.03f) }
        };
    }
    else
    {
        return {};
    }
}

float LetterWidth(char letter)
{
    if (letter == 'M')
    {
        return 0.8f;
    }
    else if (letter == 'O')
    {
        return 0.62f;
    }
    else if (letter == 'T' || letter == 'N' || letter == 'Y')
    {
        return 0.6f;
    }
    else if (letter == 'R')
    {
        return 0.57f;
    }
    else if (letter == 'L')
    {
        return 0.5f;
    }
    else if (letter == ' ')
    {
        return 0.25f;
    }
    else if (letter == '.')
    {
        return 0.12f;
    }
    else
    {
        return 0.55f;
    }
}

float TextWidth(const std::string& text)
{
    float width = -SKYWRITING_LETTER_GAP;
    for (char letter : text)
    {
        width += LetterWidth(letter) + SKYWRITING_LETTER_GAP;
    }
    return width;
}

int CountLetters(const std::string& text)
{
    int count = 0;
    for (char letter : text)
    {
        if (letter != ' ')
        {
            count++;
        }
    }
    return count;
}

void AppendPuff(std::vector<VertexPNU>& vertices, std::vector<unsigned int>& indices,
                const glm::vec3& centre, float size, float density, float brightness)
{
    unsigned int first = static_cast<unsigned int>(vertices.size());
    const glm::vec2 corners[4] =
    {
        glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(-1.0f, 1.0f)
    };
    for (const glm::vec2& corner : corners)
    {
        VertexPNU vertex{};
        vertex.position = centre;
        vertex.uv = corner;
        vertex.normal = glm::vec3(size, density, brightness);
        vertices.push_back(vertex);
    }
    indices.push_back(first);
    indices.push_back(first + 1);
    indices.push_back(first + 2);
    indices.push_back(first);
    indices.push_back(first + 2);
    indices.push_back(first + 3);
}

void AppendSkywritingLine(std::vector<VertexPNU>& vertices, std::vector<unsigned int>& indices,
                          const std::string& text, const glm::vec3& origin,
                          const glm::vec3& right, const glm::vec3& up, const glm::vec3& forward,
                          float letterHeight, float maxAge, std::mt19937& generator)
{
    const float puffSpacing = 0.028f;
    std::uniform_real_distribution<float> random(0.0f, 1.0f);

    int letterCount = CountLetters(text);
    float cursor = 0.0f;
    int letterIndex = 0;
    for (char letter : text)
    {
        float age = 0.0f;
        if (letterCount > 1)
        {
            age = maxAge * (1.0f - static_cast<float>(letterIndex) / (letterCount - 1));
        }

        for (const std::vector<glm::vec2>& stroke : LetterStrokes(letter))
        {
            for (size_t point = 0; point + 1 < stroke.size(); point++)
            {
                glm::vec2 start = stroke[point];
                glm::vec2 end = stroke[point + 1];
                int steps = std::max(1, static_cast<int>(std::ceil(glm::length(end - start) / puffSpacing)));
                for (int step = 0; step <= steps; step++)
                {
                    float jitterX = (random(generator) - 0.5f) * 0.025f * (1.0f + age);
                    float jitterY = (random(generator) - 0.5f) * 0.025f * (1.0f + age);
                    float jitterZ = (random(generator) - 0.5f) * 0.04f;
                    float sizeRoll = random(generator);
                    float densityRoll = random(generator);
                    float brightnessRoll = random(generator);

                    glm::vec2 local = glm::mix(start, end, static_cast<float>(step) / steps);
                    local.y += std::sin(local.x * 6.0f + letterIndex) * 0.03f * age;
                    glm::vec3 centre = origin
                                     + right * ((cursor + local.x + jitterX) * letterHeight)
                                     + up * ((local.y + jitterY) * letterHeight)
                                     + forward * (jitterZ * letterHeight);
                    float size = letterHeight * 0.1f * (1.0f + 0.7f * age) * (0.75f + 0.5f * sizeRoll);
                    float density = 0.7f * (1.0f - 0.45f * age) * (0.7f + 0.3f * densityRoll);
                    float brightness = 0.9f + 0.15f * brightnessRoll;
                    AppendPuff(vertices, indices, centre, size, density, brightness);
                }
            }
        }

        cursor += LetterWidth(letter) + SKYWRITING_LETTER_GAP;
        if (letter != ' ')
        {
            letterIndex++;
        }
    }
}

Mesh* CreateSkywriting()
{
    const float textDistance = 2600.0f;
    const float titleHeight = 135.0f;
    const float sloganHeight = 78.0f;
    const float lineGap = 40.0f;

    std::vector<VertexPNU> vertices;
    std::vector<unsigned int> indices;
    std::mt19937 generator(2569);

    glm::vec3 forward = DirectionFromAngles(-11.5f, 7.6f);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    float titleWidth = TextWidth(titleText) * titleHeight;
    glm::vec3 titleOrigin = photoCameraPosition + forward * textDistance
                          - right * (titleWidth * 0.5f) - up * (titleHeight * 0.5f);
    glm::vec3 sloganOrigin = titleOrigin - up * (sloganHeight + lineGap);

    AppendSkywritingLine(vertices, indices, titleText, titleOrigin, right, up, forward, titleHeight, 1.0f, generator);
    AppendSkywritingLine(vertices, indices, sloganText, sloganOrigin, right, up, forward, sloganHeight, 0.5f, generator);
    return BuildMesh(vertices, indices);
}

void AppendContrail(std::vector<VertexPNU>& vertices, std::vector<unsigned int>& indices,
                    const glm::vec3& exhaust, std::mt19937& generator)
{
    const float startDistance = 3.0f;
    const float endDistance = 700.0f;
    std::uniform_real_distribution<float> random(0.0f, 1.0f);

    float wakeCentre = 16.45f;
    if (exhaust.x < 0.0f)
    {
        wakeCentre = -16.45f;
    }

    float distance = startDistance;
    while (distance < endDistance)
    {
        float jitterX = random(generator) - 0.5f;
        float jitterY = random(generator) - 0.5f;
        float jitterZ = random(generator) - 0.5f;
        float densityRoll = random(generator);
        float brightnessRoll = random(generator);

        float radius = 0.55f + (distance - startDistance) * 0.025f;
        float spacing = radius * 0.3f;
        float merge = glm::smoothstep(40.0f, 500.0f, distance) * 0.7f;
        float turbulence = radius * (0.08f + 0.3f * glm::smoothstep(20.0f, 300.0f, distance));
        glm::vec3 centre(glm::mix(exhaust.x, wakeCentre, merge) + jitterX * turbulence,
                         exhaust.y - distance * 0.004f + jitterY * turbulence,
                         exhaust.z - distance + jitterZ * spacing);
        float density = 0.9f * glm::smoothstep(startDistance, startDistance + 15.0f, distance)
                      * (1.0f - glm::smoothstep(300.0f, endDistance, distance))
                      * (0.75f + 0.25f * densityRoll);
        AppendPuff(vertices, indices, centre, radius, density, 0.95f + 0.1f * brightnessRoll);
        distance += spacing;
    }
}

Mesh* CreateContrails()
{
    std::vector<VertexPNU> vertices;
    std::vector<unsigned int> indices;
    std::mt19937 generator(747);
    for (const glm::vec3& exhaust : engineExhausts)
    {
        AppendContrail(vertices, indices, exhaust, generator);
    }
    return BuildMesh(vertices, indices);
}

GLuint LoadTexture(const std::filesystem::path& texturePath)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(texturePath.u8string().c_str(), &width, &height, &channels, 0);
    if (data == nullptr)
    {
        std::cerr << "Failed to load texture " << texturePath.u8string() << ": " << stbi_failure_reason() << "\n";
        return 0;
    }

    GLenum format = GL_RGB;
    GLint internalFormat = GL_SRGB8;
    if (channels == 4)
    {
        format = GL_RGBA;
        internalFormat = GL_SRGB8_ALPHA8;
    }
    else if (channels == 3)
    {
        format = GL_RGB;
        internalFormat = GL_SRGB8;
    }
    else if (channels == 1)
    {
        format = GL_RED;
        internalFormat = GL_R8;
    }
    else
    {
        std::cerr << "Unsupported texture format.\n";
        stbi_image_free(data);
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (GLEW_EXT_texture_filter_anisotropic)
    {
        GLfloat maxAnisotropy = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(8.0f, maxAnisotropy));
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

ShadowMap CreateShadowMap(int size)
{
    ShadowMap shadowMap = { 0, 0, size };
    glGenFramebuffers(1, &shadowMap.framebuffer);

    glGenTextures(1, &shadowMap.depthTexture);
    glBindTexture(GL_TEXTURE_2D, shadowMap.depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColour[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap.depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Shadow map framebuffer is not complete.\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return shadowMap;
}

RenderTarget CreateRenderTarget(int width, int height)
{
    RenderTarget target = { 0, 0, width, height };
    glGenFramebuffers(1, &target.framebuffer);

    glGenTextures(1, &target.texture);
    glBindTexture(GL_TEXTURE_2D, target.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Sun occlusion framebuffer is not complete.\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return target;
}

MultisampleTarget CreateMultisampleTarget(int width, int height, int samples)
{
    MultisampleTarget target = { 0, 0, 0, width, height };
    glGenFramebuffers(1, &target.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);

    glGenRenderbuffers(1, &target.colourBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, target.colourBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, target.colourBuffer);

    glGenRenderbuffers(1, &target.depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, target.depthBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.depthBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Scene framebuffer is not complete.\n";
    }
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return target;
}

void DeleteMultisampleTarget(MultisampleTarget& target)
{
    if (target.colourBuffer != 0)
    {
        glDeleteRenderbuffers(1, &target.colourBuffer);
        target.colourBuffer = 0;
    }
    if (target.depthBuffer != 0)
    {
        glDeleteRenderbuffers(1, &target.depthBuffer);
        target.depthBuffer = 0;
    }
    if (target.framebuffer != 0)
    {
        glDeleteFramebuffers(1, &target.framebuffer);
        target.framebuffer = 0;
    }
}

void DeleteRenderTarget(RenderTarget& target)
{
    if (target.texture != 0)
    {
        glDeleteTextures(1, &target.texture);
        target.texture = 0;
    }
    if (target.framebuffer != 0)
    {
        glDeleteFramebuffers(1, &target.framebuffer);
        target.framebuffer = 0;
    }
}

void DeleteShadowMap(ShadowMap& shadowMap)
{
    if (shadowMap.depthTexture != 0)
    {
        glDeleteTextures(1, &shadowMap.depthTexture);
        shadowMap.depthTexture = 0;
    }
    if (shadowMap.framebuffer != 0)
    {
        glDeleteFramebuffers(1, &shadowMap.framebuffer);
        shadowMap.framebuffer = 0;
    }
}

Shader* LoadShader(const char* vertexFile, const char* fragmentFile)
{
    Shader* shader = new Shader();
    shader->CreateFromFiles(shaderDirectory / vertexFile, shaderDirectory / fragmentFile);
    return shader;
}

bool LoadResources(int width, int height)
{
    sceneShader = LoadShader("scene.vert", "scene.frag");
    skyShader = LoadShader("sky.vert", "sky.frag");
    glowShader = LoadShader("glow.vert", "glow.frag");
    depthShader = LoadShader("depth.vert", "depth.frag");
    smokeShader = LoadShader("smoke.vert", "smoke.frag");
    occlusionShader = LoadShader("depth.vert", "occlusion.frag");
    godRayShader = LoadShader("screen.vert", "godrays.frag");
    bloomShader = LoadShader("screen.vert", "bloom.frag");
    compositeShader = LoadShader("screen.vert", "composite.frag");
    overlayShader = LoadShader("screen.vert", "overlay.frag");
    airplaneShadow = CreateShadowMap(SHADOW_SIZE);
    sunOcclusion = CreateRenderTarget(OCCLUSION_WIDTH, OCCLUSION_HEIGHT);
    sceneTarget = CreateMultisampleTarget(width, height, SCENE_SAMPLES);
    sceneColour = CreateRenderTarget(width, height);
    bloomPing = CreateRenderTarget(BLOOM_WIDTH, BLOOM_HEIGHT);
    bloomPong = CreateRenderTarget(BLOOM_WIDTH, BLOOM_HEIGHT);

    std::filesystem::path airplanePath = modelDirectory / "Boeing 747-8I_textured.obj";
    airplane = new Mesh();
    if (!airplane->CreateMeshFromOBJ(airplanePath.u8string(), false))
    {
        std::cerr << "Failed to load model: " << airplanePath.u8string() << "\n";
        return false;
    }

    airplaneTexture = LoadTexture(textureDirectory / "faib_b7478_t_0.jpeg");
    if (airplaneTexture == 0)
    {
        return false;
    }

    skyDome = CreateSkyDome();
    cloudDeck = CreateCloudDeck();
    cumulusClouds = CreateCumulusClouds();
    mountains = CreateMountains();
    glowQuad = CreateGlowQuad();
    skywriting = CreateSkywriting();
    contrails = CreateContrails();
    return true;
}

void DeleteMesh(Mesh*& mesh)
{
    delete mesh;
    mesh = nullptr;
}

void DeleteShader(Shader*& shader)
{
    delete shader;
    shader = nullptr;
}

void Cleanup()
{
    DeleteMesh(skyDome);
    DeleteMesh(cloudDeck);
    DeleteMesh(cumulusClouds);
    DeleteMesh(mountains);
    DeleteMesh(airplane);
    DeleteMesh(glowQuad);
    DeleteMesh(skywriting);
    DeleteMesh(contrails);
    DeleteShader(sceneShader);
    DeleteShader(skyShader);
    DeleteShader(glowShader);
    DeleteShader(depthShader);
    DeleteShader(smokeShader);
    DeleteShader(occlusionShader);
    DeleteShader(godRayShader);
    DeleteShader(bloomShader);
    DeleteShader(compositeShader);
    DeleteShader(overlayShader);
    DeleteShadowMap(airplaneShadow);
    DeleteRenderTarget(sunOcclusion);
    DeleteMultisampleTarget(sceneTarget);
    DeleteRenderTarget(sceneColour);
    DeleteRenderTarget(bloomPing);
    DeleteRenderTarget(bloomPong);
    if (airplaneTexture != 0)
    {
        glDeleteTextures(1, &airplaneTexture);
        airplaneTexture = 0;
    }
}

float AirplaneBank(float time)
{
    return airplaneRoll + std::sin(time * 0.45f) * 0.012f;
}

glm::mat4 AirplaneModelMatrix(float time)
{
    glm::vec3 drift(0.0f, std::sin(time * 0.7f) * 0.35f, 0.0f);
    glm::mat4 model(1.0f);
    model = glm::translate(model, airplanePosition + drift);
    model = glm::rotate(model, airplaneYaw, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, airplanePitch, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, AirplaneBank(time), glm::vec3(0.0f, 0.0f, 1.0f));
    return model;
}

glm::mat4 LightProjection()
{
    return glm::ortho(-260.0f, 60.0f, -60.0f, 60.0f, 1.0f, 1400.0f);
}

glm::mat4 LightView(const SkySettings& sky)
{
    return glm::lookAt(airplanePosition + sky.lightDirection * 500.0f, airplanePosition, glm::vec3(0.0f, 1.0f, 0.0f));
}

void RenderAirplane(Shader* shader, const glm::mat4& view, const glm::mat4& projection, float time)
{
    SetMat4(shader, "model", AirplaneModelMatrix(time));
    SetMat4(shader, "view", view);
    SetMat4(shader, "projection", projection);
    airplane->RenderMesh();
}

void RenderShadowPass(const SkySettings& sky, float time)
{
    glm::mat4 lightView = LightView(sky);
    glm::mat4 lightProjection = LightProjection();

    glViewport(0, 0, airplaneShadow.size, airplaneShadow.size);
    glBindFramebuffer(GL_FRAMEBUFFER, airplaneShadow.framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    depthShader->UseShader();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    RenderAirplane(depthShader, lightView, lightProjection, time);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);

    smokeShader->UseShader();
    SetMat4(smokeShader, "model", AirplaneModelMatrix(time));
    SetMat4(smokeShader, "view", lightView);
    SetMat4(smokeShader, "projection", lightProjection);
    SetFloat(smokeShader, "alphaCutoff", 0.12f);
    contrails->RenderMesh();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ApplyShadowUniforms(Shader* shader, const SkySettings& sky)
{
    SetMat4(shader, "lightProjection", LightProjection());
    SetMat4(shader, "lightView", LightView(sky));
    SetInt(shader, "shadowMap", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, airplaneShadow.depthTexture);
    glActiveTexture(GL_TEXTURE0);
}

float AircraftLightLevel(const AircraftLight& light, float time)
{
    if (light.pattern == LIGHT_STEADY)
    {
        return 1.0f;
    }
    else if (light.pattern == LIGHT_STROBE)
    {
        float phase = std::fmod(time, 1.4f);
        if (phase < 0.07f)
        {
            return 1.0f;
        }
        else if (phase > 0.2f && phase < 0.27f)
        {
            return 1.0f;
        }
        else
        {
            return 0.0f;
        }
    }
    else if (light.pattern == LIGHT_BEACON)
    {
        float phase = std::fmod(time + 0.05f, 1.1f);
        if (phase < 0.15f)
        {
            return 1.0f;
        }
        else
        {
            return 0.0f;
        }
    }
    else
    {
        return 0.0f;
    }
}

void ApplySkyUniforms(Shader* shader, const SkySettings& sky)
{
    SetVec3(shader, "sunSkyDirection", sky.sunDirection);
    SetVec3(shader, "zenithColour", sky.zenithColour);
    SetVec3(shader, "midSkyColour", sky.midSkyColour);
    SetVec3(shader, "horizonSunColour", sky.horizonSunColour);
    SetVec3(shader, "horizonAwayColour", sky.horizonAwayColour);
    SetVec3(shader, "belowHorizonColour", sky.belowHorizonColour);
    SetVec3(shader, "sunGlowColour", sky.sunGlowColour);
    SetFloat(shader, "exposure", sky.exposure);
    glUniform2f(shader->GetUniformLocation("screenSize"), static_cast<float>(WIDTH), static_cast<float>(HEIGHT));
}

void ApplyLightingUniforms(Shader* shader, const SkySettings& sky)
{
    SetVec3(shader, "viewPos", eyePosition);
    SetVec3(shader, "lightDirection", sky.lightDirection);
    SetVec3(shader, "lightColour", sky.lightColour);
    SetVec3(shader, "sunLightColour", sky.sunLightColour);
    SetVec3(shader, "ambientTopColour", sky.ambientTopColour);
    SetVec3(shader, "ambientBottomColour", sky.ambientBottomColour);
    SetFloat(shader, "fogDensity", sky.fogDensity);
    SetFloat(shader, "fogBaseHeight", CLOUD_DECK_HEIGHT);
    SetFloat(shader, "fogHeightFalloff", sky.fogHeightFalloff);
}

void ApplyPointLights(Shader* shader, const glm::mat4& airplaneModel, float time)
{
    int count = 0;
    for (const AircraftLight& light : aircraftLights)
    {
        float level = AircraftLightLevel(light, time);
        if (level > 0.0f && count < MAX_POINT_LIGHTS)
        {
            glm::vec3 worldPosition = glm::vec3(airplaneModel * glm::vec4(light.localPosition, 1.0f));
            std::string index = std::to_string(count);
            SetVec3(shader, ("pointLightPositions[" + index + "]").c_str(), worldPosition);
            SetVec3(shader, ("pointLightColours[" + index + "]").c_str(), light.colour * level * light.glowSize * 2.0f);
            count++;
        }
    }
    SetInt(shader, "pointLightCount", count);
}

void RenderSunOcclusionPass(const glm::mat4& view, const glm::mat4& projection, const SkySettings& sky, float time)
{
    glViewport(0, 0, sunOcclusion.width, sunOcclusion.height);
    glBindFramebuffer(GL_FRAMEBUFFER, sunOcclusion.framebuffer);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    skyShader->UseShader();
    SetMat4(skyShader, "view", view);
    SetMat4(skyShader, "projection", projection);
    ApplySkyUniforms(skyShader, sky);
    SetInt(skyShader, "occlusionPass", 1);
    skyDome->RenderMesh();

    occlusionShader->UseShader();
    SetMat4(occlusionShader, "model", glm::mat4(1.0f));
    SetMat4(occlusionShader, "view", view);
    SetMat4(occlusionShader, "projection", projection);
    SetFloat(occlusionShader, "maskValue", 0.0f);
    mountains->RenderMesh();
    cloudDeck->RenderMesh();
    cumulusClouds->RenderMesh();
    SetFloat(occlusionShader, "maskValue", 1.0f);
    RenderAirplane(occlusionShader, view, projection, time);

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool SunScreenPosition(const glm::mat4& view, const glm::mat4& projection, const SkySettings& sky, glm::vec2& screen)
{
    glm::vec3 sunInView = glm::mat3(view) * sky.sunDirection;
    glm::vec4 sunClip = projection * glm::vec4(sunInView, 1.0f);
    if (sunClip.w <= 0.0f)
    {
        return false;
    }
    screen = glm::vec2(sunClip) / sunClip.w;
    return true;
}

void DrawGodRays(const glm::mat4& view, const glm::mat4& projection, const SkySettings& sky)
{
    glm::vec2 sunScreen(0.0f);
    if (!SunScreenPosition(view, projection, sky, sunScreen))
    {
        return;
    }
    float offScreen = std::max(std::fabs(sunScreen.x), std::fabs(sunScreen.y));
    float fade = 1.0f - glm::smoothstep(1.0f, 1.6f, offScreen);
    if (fade <= 0.0f)
    {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    godRayShader->UseShader();
    SetInt(godRayShader, "occlusionMap", 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, sunOcclusion.texture);
    glActiveTexture(GL_TEXTURE0);
    glUniform2f(godRayShader->GetUniformLocation("sunScreenPosition"), sunScreen.x * 0.5f + 0.5f, sunScreen.y * 0.5f + 0.5f);
    SetVec3(godRayShader, "rayColour", glm::vec3(1.0f, 0.72f, 0.42f));
    SetFloat(godRayShader, "rayExposure", 1.2f * fade);
    SetFloat(godRayShader, "decay", 0.99f);
    SetFloat(godRayShader, "density", 0.96f);
    SetFloat(godRayShader, "weight", 0.025f);
    glowQuad->RenderMesh();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void DrawSky(const glm::mat4& view, const glm::mat4& projection, const SkySettings& sky)
{
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    skyShader->UseShader();
    SetMat4(skyShader, "view", view);
    SetMat4(skyShader, "projection", projection);
    ApplySkyUniforms(skyShader, sky);
    SetInt(skyShader, "occlusionPass", 0);
    SetVec3(skyShader, "sunDiscColour", sky.sunDiscColour);
    skyDome->RenderMesh();
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void DrawScenery(const glm::mat4& view, const glm::mat4& projection, const SkySettings& sky)
{
    sceneShader->UseShader();
    SetMat4(sceneShader, "model", glm::mat4(1.0f));
    SetMat4(sceneShader, "view", view);
    SetMat4(sceneShader, "projection", projection);
    ApplySkyUniforms(sceneShader, sky);
    ApplyLightingUniforms(sceneShader, sky);
    ApplyShadowUniforms(sceneShader, sky);
    SetInt(sceneShader, "pointLightCount", 0);
    SetInt(sceneShader, "useTexture", 0);

    SetInt(sceneShader, "materialType", MATERIAL_MOUNTAIN);
    SetFloat(sceneShader, "shininess", 24.0f);
    SetFloat(sceneShader, "specularStrength", 0.3f);
    mountains->RenderMesh();

    SetInt(sceneShader, "materialType", MATERIAL_CLOUD);
    SetVec3(sceneShader, "objectColour", glm::vec3(0.95f, 0.93f, 0.97f));
    cloudDeck->RenderMesh();
    cumulusClouds->RenderMesh();
}

void DrawSmoke(Mesh* smoke, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    smokeShader->UseShader();
    SetMat4(smokeShader, "model", model);
    SetMat4(smokeShader, "view", view);
    SetMat4(smokeShader, "projection", projection);
    SetFloat(smokeShader, "alphaCutoff", 0.0f);
    SetVec3(smokeShader, "smokeColour", glm::vec3(1.0f, 0.92f, 0.86f));
    SetVec3(smokeShader, "smokeShadowColour", glm::vec3(0.72f, 0.68f, 0.85f));
    smoke->RenderMesh();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void DrawAirplane(const glm::mat4& view, const glm::mat4& projection, const SkySettings& sky, float time)
{
    sceneShader->UseShader();
    ApplySkyUniforms(sceneShader, sky);
    ApplyLightingUniforms(sceneShader, sky);
    ApplyShadowUniforms(sceneShader, sky);
    ApplyPointLights(sceneShader, AirplaneModelMatrix(time), time);

    SetInt(sceneShader, "materialType", MATERIAL_PLANE);
    SetFloat(sceneShader, "shininess", 90.0f);
    SetFloat(sceneShader, "specularStrength", 1.2f);
    SetVec3(sceneShader, "objectColour", glm::vec3(0.8f));
    SetVec3(sceneShader, "cockpitLightColour", glm::vec3(2.2f, 1.0f, 0.35f));
    SetInt(sceneShader, "useTexture", 1);
    SetInt(sceneShader, "texture_data", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, airplaneTexture);
    RenderAirplane(sceneShader, view, projection, time);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DrawAircraftLights(const glm::mat4& view, const glm::mat4& projection, float time)
{
    glm::mat4 model = AirplaneModelMatrix(time);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);

    glowShader->UseShader();
    SetMat4(glowShader, "view", view);
    SetMat4(glowShader, "projection", projection);
    SetFloat(glowShader, "coreSize", 0.02f);

    for (const AircraftLight& light : aircraftLights)
    {
        float level = AircraftLightLevel(light, time);
        if (level > 0.0f)
        {
            glm::vec3 lightPosition = glm::vec3(model * glm::vec4(light.localPosition, 1.0f));
            glm::vec3 towardCamera = glm::normalize(eyePosition - lightPosition);
            SetVec3(glowShader, "centre", lightPosition + towardCamera * light.glowSize * 0.5f);
            SetFloat(glowShader, "size", light.glowSize);
            SetVec3(glowShader, "glowColour", light.colour);
            SetFloat(glowShader, "intensity", level);
            glowQuad->RenderMesh();
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void DrawSunFlare(const glm::mat4& view, const glm::mat4& projection, const SkySettings& sky)
{
    glm::vec2 sunScreen(0.0f);
    if (!SunScreenPosition(view, projection, sky, sunScreen))
    {
        return;
    }
    else if (std::fabs(sunScreen.x) > 1.3f || std::fabs(sunScreen.y) > 1.3f)
    {
        return;
    }

    float aspect = static_cast<float>(WIDTH) / HEIGHT;
    glm::mat4 identity(1.0f);
    glm::mat4 screenProjection = glm::scale(identity, glm::vec3(1.0f / aspect, 1.0f, 1.0f));

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glowShader->UseShader();
    SetMat4(glowShader, "view", identity);
    SetMat4(glowShader, "projection", screenProjection);
    SetFloat(glowShader, "coreSize", 0.35f);

    for (const FlareGhost& ghost : flareGhosts)
    {
        glm::vec2 position = sunScreen * (1.0f - ghost.offset);
        SetVec3(glowShader, "centre", glm::vec3(position.x * aspect, position.y, 0.0f));
        SetFloat(glowShader, "size", ghost.size);
        SetVec3(glowShader, "glowColour", ghost.colour);
        SetFloat(glowShader, "intensity", ghost.intensity);
        glowQuad->RenderMesh();
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void ResolveScene()
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneTarget.framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sceneColour.framebuffer);
    glBlitFramebuffer(0, 0, sceneTarget.width, sceneTarget.height,
                      0, 0, sceneColour.width, sceneColour.height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RunBloomPass(const RenderTarget& destination, const RenderTarget& source, int mode, const glm::vec2& direction)
{
    glBindFramebuffer(GL_FRAMEBUFFER, destination.framebuffer);
    glViewport(0, 0, destination.width, destination.height);
    SetInt(bloomShader, "bloomMode", mode);
    glUniform2f(bloomShader->GetUniformLocation("blurDirection"), direction.x, direction.y);
    glBindTexture(GL_TEXTURE_2D, source.texture);
    glowQuad->RenderMesh();
}

void RenderBloom()
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    bloomShader->UseShader();
    SetInt(bloomShader, "sourceTexture", 3);
    SetFloat(bloomShader, "threshold", 0.86f);
    glActiveTexture(GL_TEXTURE3);

    glm::vec2 horizontal(1.0f / BLOOM_WIDTH, 0.0f);
    glm::vec2 vertical(0.0f, 1.0f / BLOOM_HEIGHT);
    RunBloomPass(bloomPing, sceneColour, BLOOM_BRIGHT_PASS, glm::vec2(0.0f));
    for (int pass = 0; pass < 2; pass++)
    {
        RunBloomPass(bloomPong, bloomPing, BLOOM_BLUR, horizontal);
        RunBloomPass(bloomPing, bloomPong, BLOOM_BLUR, vertical);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
}

void DrawComposite(int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);

    compositeShader->UseShader();
    SetInt(compositeShader, "sceneTexture", 3);
    SetInt(compositeShader, "bloomTexture", 4);
    SetFloat(compositeShader, "bloomStrength", 0.6f);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, sceneColour.texture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, bloomPing.texture);
    glowQuad->RenderMesh();

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_DEPTH_TEST);
}

bool SaveScreenshot(const std::filesystem::path& path, int width, int height)
{
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    stbi_flip_vertically_on_write(1);
    if (stbi_write_png(path.u8string().c_str(), width, height, 3, pixels.data(), width * 3) == 0)
    {
        std::cerr << "Failed to save screenshot: " << path.u8string() << "\n";
        return false;
    }
    std::cout << "Saved screenshot: " << path.u8string() << "\n";
    return true;
}

glm::vec3 CameraDirection(const Camera& eye)
{
    glm::vec3 direction;
    direction.x = std::cos(glm::radians(eye.yaw)) * std::cos(glm::radians(eye.pitch));
    direction.y = std::sin(glm::radians(eye.pitch));
    direction.z = std::sin(glm::radians(eye.yaw)) * std::cos(glm::radians(eye.pitch));
    return glm::normalize(direction);
}

glm::mat4 CameraViewMatrix(const Camera& eye)
{
    glm::vec3 direction = CameraDirection(eye);
    glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, direction));

    glm::mat4 cameraPosMat(1.0f);
    cameraPosMat[3][0] = -eye.position.x;
    cameraPosMat[3][1] = -eye.position.y;
    cameraPosMat[3][2] = -eye.position.z;

    glm::mat4 cameraRotateMat(1.0f);
    cameraRotateMat[0] = glm::vec4(right.x, up.x, -direction.x, 0.0f);
    cameraRotateMat[1] = glm::vec4(right.y, up.y, -direction.y, 0.0f);
    cameraRotateMat[2] = glm::vec4(right.z, up.z, -direction.z, 0.0f);

    return cameraRotateMat * cameraPosMat;
}

Camera PhotoCamera()
{
    glm::vec3 direction = glm::normalize(photoCameraTarget - photoCameraPosition);
    Camera photo;
    photo.position = photoCameraPosition;
    photo.yaw = glm::degrees(std::atan2(direction.z, direction.x));
    photo.pitch = glm::degrees(std::asin(direction.y));
    return photo;
}

void UpdateMouseLook(GLFWwindow* window, Camera& eye, MouseLook& mouse)
{
    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    if (mouse.firstMove)
    {
        mouse.lastX = mouseX;
        mouse.lastY = mouseY;
        mouse.firstMove = false;
    }

    float sensitivity = 0.1f;
    float xOffset = static_cast<float>(mouseX - mouse.lastX) * sensitivity;
    float yOffset = static_cast<float>(mouseY - mouse.lastY) * sensitivity;
    mouse.lastX = mouseX;
    mouse.lastY = mouseY;

    eye.yaw += xOffset;
    eye.pitch -= yOffset;
    if (eye.pitch > 89.0f)
    {
        eye.pitch = 89.0f;
    }
    else if (eye.pitch < -89.0f)
    {
        eye.pitch = -89.0f;
    }
}

void UpdateCameraMovement(GLFWwindow* window, Camera& eye, float deltaTime)
{
    float moveSpeed = 40.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        moveSpeed = 320.0f;
    }

    glm::vec3 direction = CameraDirection(eye);
    glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    float distance = moveSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        eye.position += direction * distance;
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        eye.position -= direction * distance;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        eye.position -= right * distance;
    }
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        eye.position += right * distance;
    }

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        eye.position += worldUp * distance;
    }
    else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        eye.position -= worldUp * distance;
    }
}

bool KeyPressedOnce(GLFWwindow* window, int key, bool& held)
{
    bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
    bool triggered = false;
    if (pressed && !held)
    {
        triggered = true;
    }
    held = pressed;
    return triggered;
}

ViewSetup AttachedViewSetup(const AttachedView& attached, float time)
{
    glm::mat4 model = AirplaneModelMatrix(time);
    glm::vec3 eye = glm::vec3(model * glm::vec4(attached.localEye, 1.0f));
    glm::vec3 target = glm::vec3(model * glm::vec4(attached.localTarget, 1.0f));
    glm::vec3 up = glm::normalize(glm::vec3(model * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));

    ViewSetup setup;
    setup.view = glm::lookAt(eye, target, up);
    setup.eye = eye;
    setup.fieldOfView = attached.fieldOfView;
    setup.nearPlane = attached.nearPlane;
    setup.overlay = attached.overlay;
    return setup;
}

ViewSetup ComputeViewSetup(int viewMode, float time)
{
    if (viewMode == VIEW_CHASE)
    {
        return AttachedViewSetup(chaseView, time);
    }
    else if (viewMode == VIEW_WINDOW)
    {
        return AttachedViewSetup(windowView, time);
    }
    else if (viewMode == VIEW_COCKPIT)
    {
        return AttachedViewSetup(cockpitView, time);
    }
    else
    {
        ViewSetup setup;
        setup.view = CameraViewMatrix(camera);
        setup.eye = camera.position;
        setup.fieldOfView = 38.0f;
        setup.nearPlane = 2.0f;
        setup.overlay = OVERLAY_NONE;
        return setup;
    }
}

Camera CameraFromView(const ViewSetup& setup)
{
    glm::vec3 forward = -glm::vec3(setup.view[0][2], setup.view[1][2], setup.view[2][2]);
    Camera result;
    result.position = setup.eye;
    result.yaw = glm::degrees(std::atan2(forward.z, forward.x));
    result.pitch = glm::degrees(std::asin(glm::clamp(forward.y, -1.0f, 1.0f)));
    return result;
}

const char* ViewTitle(int viewMode)
{
    if (viewMode == VIEW_CHASE)
    {
        return "Assignment2_67050651 - FUTURE ME  [2 Chase view | 1 2 3 4 views | C free | P capture]";
    }
    else if (viewMode == VIEW_WINDOW)
    {
        return "Assignment2_67050651 - FUTURE ME  [3 Passenger window | 1 2 3 4 views | C free | P capture]";
    }
    else if (viewMode == VIEW_COCKPIT)
    {
        return "Assignment2_67050651 - FUTURE ME  [4 Captain's seat | 1 2 3 4 views | C free | P capture]";
    }
    else if (viewMode == VIEW_FREE)
    {
        return "Assignment2_67050651 - FUTURE ME  [Free camera: WASD QE mouse, Shift fast | C stop | R reset]";
    }
    else
    {
        return "Assignment2_67050651 - FUTURE ME  [1 Photo | 2 Chase | 3 Window | 4 Cockpit | C free | P capture]";
    }
}

void SetViewMode(GLFWwindow* window, InputState& input, int viewMode)
{
    input.viewMode = viewMode;
    input.mouse.firstMove = true;
    if (viewMode == VIEW_FREE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    glfwSetWindowTitle(window, ViewTitle(viewMode));
}

void HandleInput(GLFWwindow* window, InputState& input, float deltaTime, float time)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    if (KeyPressedOnce(window, GLFW_KEY_P, input.captureHeld))
    {
        input.captureRequested = true;
    }

    if (KeyPressedOnce(window, GLFW_KEY_1, input.viewHeld[0]))
    {
        camera = PhotoCamera();
        SetViewMode(window, input, VIEW_PHOTO);
    }
    else if (KeyPressedOnce(window, GLFW_KEY_2, input.viewHeld[1]))
    {
        SetViewMode(window, input, VIEW_CHASE);
    }
    else if (KeyPressedOnce(window, GLFW_KEY_3, input.viewHeld[2]))
    {
        SetViewMode(window, input, VIEW_WINDOW);
    }
    else if (KeyPressedOnce(window, GLFW_KEY_4, input.viewHeld[3]))
    {
        SetViewMode(window, input, VIEW_COCKPIT);
    }
    else if (KeyPressedOnce(window, GLFW_KEY_C, input.freeHeld))
    {
        if (input.viewMode == VIEW_FREE)
        {
            SetViewMode(window, input, VIEW_PHOTO);
        }
        else
        {
            camera = CameraFromView(ComputeViewSetup(input.viewMode, time));
            SetViewMode(window, input, VIEW_FREE);
        }
    }
    else if (KeyPressedOnce(window, GLFW_KEY_R, input.resetHeld))
    {
        camera = PhotoCamera();
        SetViewMode(window, input, VIEW_PHOTO);
    }

    if (input.viewMode == VIEW_FREE)
    {
        UpdateMouseLook(window, camera, input.mouse);
        UpdateCameraMovement(window, camera, deltaTime);
    }
}

void DrawOverlay(int overlay, int width, int height, float time)
{
    if (overlay == OVERLAY_NONE)
    {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    overlayShader->UseShader();
    SetInt(overlayShader, "overlayMode", overlay);
    glUniform2f(overlayShader->GetUniformLocation("screenSize"), static_cast<float>(width), static_cast<float>(height));
    SetFloat(overlayShader, "bankAngle", AirplaneBank(time));
    glowQuad->RenderMesh();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

int main(int argc, char** argv)
{
    bool autoCapture = false;
    std::filesystem::path capturePath = projectDirectory / captureFileName;
    if (argc >= 3 && std::strcmp(argv[1], "--capture") == 0)
    {
        autoCapture = true;
        capturePath = std::filesystem::u8path(argv[2]);
    }
    else if (argc >= 2 && std::strcmp(argv[1], "--capture") == 0)
    {
        autoCapture = true;
    }

    int startView = VIEW_PHOTO;
    if (argc >= 4)
    {
        int requestedView = std::atoi(argv[3]);
        if (requestedView == 2)
        {
            startView = VIEW_CHASE;
        }
        else if (requestedView == 3)
        {
            startView = VIEW_WINDOW;
        }
        else if (requestedView == 4)
        {
            startView = VIEW_COCKPIT;
        }
    }

    if (!glfwInit())
    {
        std::cerr << "GLFW initialisation failed.\n";
        return 1;
    }
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    Window mainWindow(WIDTH, HEIGHT, 3, 3);
    if (mainWindow.initialise() != 0)
    {
        return 1;
    }
    glEnable(GL_MULTISAMPLE);

    if (!LoadResources(mainWindow.getBufferWidth(), mainWindow.getBufferHeight()))
    {
        Cleanup();
        return 1;
    }

    SkySettings sky = CreateSunriseSky();
    int width = mainWindow.getBufferWidth();
    int height = mainWindow.getBufferHeight();
    float aspect = static_cast<float>(width) / static_cast<float>(height);

    camera = PhotoCamera();
    InputState input = { VIEW_PHOTO, false, false, false, false, { false, false, false, false }, { true, 0.0, 0.0 } };
    SetViewMode(mainWindow.getWindow(), input, startView);

    int frameCount = 0;
    float lastTime = static_cast<float>(glfwGetTime());

    while (!mainWindow.getShouldClose())
    {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        float time = currentTime;
        if (autoCapture)
        {
            time = CAPTURE_TIME;
        }

        glfwPollEvents();
        HandleInput(mainWindow.getWindow(), input, deltaTime, time);

        ViewSetup setup = ComputeViewSetup(input.viewMode, time);
        eyePosition = setup.eye;
        glm::mat4 view = setup.view;
        glm::mat4 projection = glm::perspective(glm::radians(setup.fieldOfView), aspect, setup.nearPlane, 40000.0f);

        RenderShadowPass(sky, time);
        RenderSunOcclusionPass(view, projection, sky, time);

        glBindFramebuffer(GL_FRAMEBUFFER, sceneTarget.framebuffer);
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        DrawSky(view, projection, sky);
        DrawScenery(view, projection, sky);
        DrawSmoke(skywriting, glm::mat4(1.0f), view, projection);
        DrawAirplane(view, projection, sky, time);
        DrawSmoke(contrails, AirplaneModelMatrix(time), view, projection);
        DrawAircraftLights(view, projection, time);
        DrawSunFlare(view, projection, sky);

        ResolveScene();
        RenderBloom();
        DrawComposite(width, height);
        DrawGodRays(view, projection, sky);
        DrawOverlay(setup.overlay, width, height, time);

        if (autoCapture && frameCount == 5)
        {
            input.captureRequested = true;
        }
        if (input.captureRequested)
        {
            SaveScreenshot(capturePath, mainWindow.getBufferWidth(), mainWindow.getBufferHeight());
            input.captureRequested = false;
            if (autoCapture)
            {
                glfwSetWindowShouldClose(mainWindow.getWindow(), GLFW_TRUE);
            }
        }

        mainWindow.swapBuffers();
        frameCount++;
    }

    Cleanup();
    return 0;
}
