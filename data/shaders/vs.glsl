#version 450

layout (location = 0) in vec3 vsIn_Position;

layout (location = 0) out vec3 vsOut_Colour;

layout (push_constant) uniform PushConstants
{
    mat4 ModelViewProjection;
} u_PushConstants;

void main()
{
    gl_Position = u_PushConstants.ModelViewProjection * vec4(vsIn_Position, 1);
    vsOut_Colour = vec3(1, 0, 1);

    gl_PointSize = 8.0f;
}
