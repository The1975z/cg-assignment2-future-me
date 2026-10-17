#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

out vec2 Offset;
out float Density;
out float Brightness;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);
    float size = aNormal.x;
    vec3 centre = vec3(model * vec4(pos, 1.0));
    vec3 worldPosition = centre + (cameraRight * aTexCoord.x + cameraUp * aTexCoord.y) * size;
    gl_Position = projection * view * vec4(worldPosition, 1.0);
    Offset = aTexCoord;
    Density = aNormal.y;
    Brightness = aNormal.z;
}
