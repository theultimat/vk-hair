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
    uint u_HairStrandsPerTriangle;
    uint u_TrianglesPerGroup;
    uint u_HairSmoothFactor;
};

shared vec3 PositionBuffer[VHS_COMPUTE_LOCAL_SIZE];
shared vec3 BarycentricCoords[VHS_COMPUTE_LOCAL_SIZE];
shared uint HairRootIndexBuffer[VHS_COMPUTE_LOCAL_SIZE];

vec3 cubicHermite(vec3 A, vec3 B, vec3 C, vec3 D, float t)
{
    vec3 a = -0.5f * A + 1.5f * B - 1.5f * C + 0.5f * D;
    vec3 b = A - 2.5f * B + 2.0f * C - 0.5f * D;
    vec3 c = -0.5f * A + C * 0.5f;
    vec3 d = B;

    return a * t * t * t + b * t * t + c * t + d;
}

void main()
{
    uint numIndices = u_TrianglesPerGroup * 3;

    // Because we don't completely fill the local group with meaninful work we need to calculate the global ID
    // manually to find the correct offset within the buffers.
    uint lid = gl_LocalInvocationID.x;
    uint gid = gl_WorkGroupID.x * numIndices + lid;

    // Load the barycentric coordinates into shared memory.
    if (lid < u_HairStrandsPerTriangle)
    {
        uint barycentricOffset = u_HairTotalParticles * 3 * 2 + u_TrianglesPerGroup * gl_NumWorkGroups.x * 3;

        BarycentricCoords[lid].x = ParticleStateBuffer[barycentricOffset + 3 * lid + 0];
        BarycentricCoords[lid].y = ParticleStateBuffer[barycentricOffset + 3 * lid + 1];
        BarycentricCoords[lid].z = ParticleStateBuffer[barycentricOffset + 3 * lid + 2];
    }

    // Load the root vertex triangle indices into shared memory.
    if (lid < numIndices)
    {
        uint indicesOffset = u_HairTotalParticles * 3 * 2;

        HairRootIndexBuffer[lid] = uint(ParticleStateBuffer[indicesOffset + gid]);
    }

    barrier();

    // Load the initial particle positions from the state buffer.
    uint numInitialParticles = u_HairParticlesPerStrand * u_TrianglesPerGroup * 3;

    if (lid < numInitialParticles)
    {
        // 0: triangle 0 strand 0 particle 0
        // 1: triangle 0 strand 0 particle 1
        // 2: triangle 0 strand 0 particle 2
        // ...
        // x: triangle 0 strand 1 particle 0
        // ...
        // y: triangle 1 strand 0 particle 0
        // ...

        uint triangleIndex = lid / (u_HairParticlesPerStrand * 3);
        uint strandIndex = (lid / u_HairParticlesPerStrand) % 3;
        uint particleIndex = lid % u_HairParticlesPerStrand;

        uint indexOffset = HairRootIndexBuffer[triangleIndex * 3 + strandIndex] * u_HairParticlesPerStrand;
        uint particleOffset = indexOffset + particleIndex;

        PositionBuffer[lid].x = ParticleStateBuffer[particleOffset + u_HairTotalParticles * 0];
        PositionBuffer[lid].y = ParticleStateBuffer[particleOffset + u_HairTotalParticles * 1];
        PositionBuffer[lid].z = ParticleStateBuffer[particleOffset + u_HairTotalParticles * 2];
    }
    else
    {
        PositionBuffer[lid] = vec3(0);
    }

    // Compute the new global ID for the vertices.
    gid = gl_WorkGroupID.x * u_TrianglesPerGroup * u_HairParticlesPerStrand * u_HairStrandsPerTriangle * u_HairSmoothFactor + lid;

    barrier();

    // Find the indices of the original particle for the interpolation step and load them from shared memory.
    uint triangleIndex = lid / (u_HairParticlesPerStrand * u_HairSmoothFactor * u_HairStrandsPerTriangle);
    uint interpParticleIndex = lid % (u_HairParticlesPerStrand * u_HairSmoothFactor);
    uint particleIndex = interpParticleIndex / u_HairSmoothFactor;

    // Calculate indices for interpolation along the strand.
    uint indexB = particleIndex;
    uint indexA = (indexB == 0) ? 0 : (indexB - 1);
    uint indexC = min(indexB + 1, u_HairParticlesPerStrand - 1);
    uint indexD = min(indexC + 1, u_HairParticlesPerStrand - 1);

    uint bi0 = triangleIndex * u_HairParticlesPerStrand * 3 + u_HairParticlesPerStrand * 0;
    uint bi1 = triangleIndex * u_HairParticlesPerStrand * 3 + u_HairParticlesPerStrand * 1;
    uint bi2 = triangleIndex * u_HairParticlesPerStrand * 3 + u_HairParticlesPerStrand * 2;

    // vec3 b0 = PositionBuffer[triangleIndex * u_HairParticlesPerStrand * 3 + u_HairParticlesPerStrand * 0 + particleIndex];
    // vec3 b1 = PositionBuffer[triangleIndex * u_HairParticlesPerStrand * 3 + u_HairParticlesPerStrand * 1 + particleIndex];
    // vec3 b2 = PositionBuffer[triangleIndex * u_HairParticlesPerStrand * 3 + u_HairParticlesPerStrand * 2 + particleIndex];

    // Load the barycentric coordinate and compute the new interpolated particle base positions.
    vec3 b = BarycentricCoords[lid / (u_HairParticlesPerStrand * u_HairSmoothFactor)];

    vec3 A = b.x * PositionBuffer[bi0 + indexA] + b.y * PositionBuffer[bi1 + indexA] + b.z * PositionBuffer[bi2 + indexA];
    vec3 B = b.x * PositionBuffer[bi0 + indexB] + b.y * PositionBuffer[bi1 + indexB] + b.z * PositionBuffer[bi2 + indexB];
    vec3 C = b.x * PositionBuffer[bi0 + indexC] + b.y * PositionBuffer[bi1 + indexC] + b.z * PositionBuffer[bi2 + indexC];
    vec3 D = b.x * PositionBuffer[bi0 + indexD] + b.y * PositionBuffer[bi1 + indexD] + b.z * PositionBuffer[bi2 + indexD];

    float t = interpParticleIndex / float(u_HairParticlesPerStrand * u_HairSmoothFactor);
    vec3 p = cubicHermite(A, B, C, D, t);

    //vec3 p = b0 * b.x + b1 * b.y + b2 * b.z;

    // Compute the vector perpendicular to the camera and hair direction.
    bool valid = lid < (u_TrianglesPerGroup * u_HairParticlesPerStrand * u_HairStrandsPerTriangle * u_HairSmoothFactor);
    bool end = particleIndex == (u_HairParticlesPerStrand - 1);

    uint i0 = end ? (particleIndex - 1) : particleIndex;
    uint i1 = end ? particleIndex : (particleIndex + 1);

    vec3 v0 = PositionBuffer[triangleIndex * u_HairParticlesPerStrand * 3 + i0];
    vec3 v1 = PositionBuffer[triangleIndex * u_HairParticlesPerStrand * 3 + i1];

    vec3 perp = u_HairDrawRadius * normalize(cross(v1 - v0, u_CameraFront));

    // Create the two vertices.
    vec3 p0 = p - perp;
    vec3 p1 = p + perp;

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
