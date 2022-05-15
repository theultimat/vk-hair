#version 450

layout (local_size_x = 256) in;

layout (set = 0, binding = 0, std430) buffer buffer_data
{
    int data[];
} b_Data;

void main()
{
    uint id = gl_GlobalInvocationID.x;

    if (id < 10)
    {
        b_Data.data[id] *= 2;
    }
}
