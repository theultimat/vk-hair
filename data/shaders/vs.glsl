#version 450

layout (location = 0) in vec3 vsIn_Position;

layout (location = 0) out vec3 vsOut_Colour;

layout (push_constant) uniform PushConstants
{
    mat4 ModelViewProjection;
} u_PushConstants;

float rand(vec2 seed)
{
    return fract(sin(dot(seed, vec2(12.989, 78.233))) * 43758.5453);
}

vec3 randcol(uint id)
{
    vec3 col;

    col.r = rand(vec2(id, id + 1000));
    col.g = rand(vec2(id, id + 2000));
    col.b = rand(vec2(id, id + 3000));

    return col;
}

void main()
{
    gl_Position = u_PushConstants.ModelViewProjection * vec4(vsIn_Position, 1);
    vsOut_Colour = randcol(gl_VertexIndex / 8);

    gl_PointSize = 8.0f;
}
