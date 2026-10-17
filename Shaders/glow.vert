#version 330 core

layout(location = 0) in vec3 pos;

out vec2 Offset;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 centre;
uniform float size;

void main()
{
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);
    vec3 worldPosition = centre + (cameraRight * pos.x + cameraUp * pos.y) * size;
    gl_Position = projection * view * vec4(worldPosition, 1.0);
    Offset = pos.xy;
}
