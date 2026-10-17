#version 330 core

layout(location = 0) in vec3 pos;

out vec2 TexCoord;

void main()
{
    TexCoord = pos.xy * 0.5 + 0.5;
    gl_Position = vec4(pos.xy, 0.0, 1.0);
}
