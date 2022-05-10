#version 450

layout (location = 0) in vec2 vsIn_Position;
layout (location = 1) in vec3 vsIn_Colour;

layout (location = 0) out vec3 vsOut_Colour;

void main()
{
    gl_Position = vec4(vsIn_Position, 0, 1);
    vsOut_Colour = vsIn_Colour;
}
