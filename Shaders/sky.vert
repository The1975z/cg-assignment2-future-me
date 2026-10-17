#version 330 core

layout(location = 0) in vec3 pos;

out vec3 Direction;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    Direction = pos;
    vec4 clipPosition = projection * mat4(mat3(view)) * vec4(pos, 1.0);
    gl_Position = clipPosition.xyww;
}
