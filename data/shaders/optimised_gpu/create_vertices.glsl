#version 450

layout (local_size_x = VHS_COMPUTE_LOCAL_SIZE) in;

layout (std430, set = 0, binding = VHS_PARTICLE_BUFFER_BINDING) readonly buffer ssbo
{
    float ParticleStateBuffer[];
};

layout (std430, set = 0, binding = VHS_VERTEX_BUFFER_BINDING) buffer vbo
{
    float VertexBuffer[];
};

layout (push_constant) uniform ubo
{
    vec3 u_CameraFront;
    float u_HairDrawRadius;
    uint u_HairTotalParticles;
    uint u_HairParticlesPerStrand;
};

shared vec3 PositionBuffer[VHS_COMPUTE_LOCAL_SIZE];

void main()
{
    uint gid = gl_GlobalInvocationID.x;
    uint lid = gl_LocalInvocationID.x;

    bool valid = gid < u_HairTotalParticles;
    bool end = (gid % u_HairParticlesPerStrand) == (u_HairParticlesPerStrand - 1);

    // Read particle position from the input buffer.
    PositionBuffer[lid].x = valid ? ParticleStateBuffer[gid + u_HairTotalParticles * 0] : 0.0f;
    PositionBuffer[lid].y = valid ? ParticleStateBuffer[gid + u_HairTotalParticles * 1] : 0.0f;
    PositionBuffer[lid].z = valid ? ParticleStateBuffer[gid + u_HairTotalParticles * 2] : 0.0f;

    // Calculate the vector perpendicular to camera and hair direction.
    uint i0 = end ? (lid - 1) : lid;
    uint i1 = end ? lid : (lid + 1);

    vec3 v0 = valid ? PositionBuffer[i0] : vec3(0);
    vec3 v1 = valid ? PositionBuffer[i1] : vec3(1);

    vec3 perp = u_HairDrawRadius * normalize(cross(v1 - v0, u_CameraFront));

    // Create the two vertices.
    vec3 p0 = PositionBuffer[lid] - perp;
    vec3 p1 = PositionBuffer[lid] + perp;

    if (valid)
    {
        VertexBuffer[6 * gid + 0] = p0.x;
        VertexBuffer[6 * gid + 1] = p0.y;
        VertexBuffer[6 * gid + 2] = p0.z;

        VertexBuffer[6 * gid + 3] = p1.x;
        VertexBuffer[6 * gid + 4] = p1.y;
        VertexBuffer[6 * gid + 5] = p1.z;
    }
}
