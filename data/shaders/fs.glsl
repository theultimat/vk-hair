#version 450

layout (location = 0) in vec3 fsIn_Colour;

layout (location = 0) out vec4 fsOut_Colour;

void main()
{
    fsOut_Colour = vec4(fsIn_Colour, 1.0f);
}
