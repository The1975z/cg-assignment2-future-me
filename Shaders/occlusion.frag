#version 330 core

out vec4 colour;

uniform float maskValue;

void main()
{
    colour = vec4(0.0, maskValue, 0.0, 1.0);
}
