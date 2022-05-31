#version 450

layout (local_size_x = VHS_COMPUTE_LOCAL_SIZE) in;

layout (std430, set = 0, binding = VHS_PARTICLE_BUFFER_BINDING) buffer ssbo
{
    float ParticleStateBuffer[];
};

layout (push_constant) uniform ubo
{
    vec3 u_ExternalForces;
    float u_HairParticleSeparation;
    float u_DeltaTime;
    float u_DeltaTimeSq;
    float u_DeltaTimeInv;
    float u_DampingFactor;
    uint u_HairTotalParticles;
    uint u_HairParticlesPerStrand;
    uint u_FtlIterations;
};

shared vec3 PositionBuffer[VHS_COMPUTE_LOCAL_SIZE];
shared vec3 VelocityBuffer[VHS_COMPUTE_LOCAL_SIZE];

void main()
{
    uint gid = gl_GlobalInvocationID.x;
    uint lid = gl_LocalInvocationID.x;

    uint plid = lid - 1;
    uint nlid = lid + 1;

    bool valid = gid < u_HairTotalParticles;
    bool root = (gid % u_HairParticlesPerStrand) == 0;
    bool correct = (gid % u_HairParticlesPerStrand) != (u_HairParticlesPerStrand - 1);

    // Load particle position and velocity into shared memory.
    uint offsetPosition = gid;
    uint offsetVelocity = gid + u_HairTotalParticles * u_HairParticlesPerStrand * 3;

    PositionBuffer[lid].x = valid ? ParticleStateBuffer[offsetPosition + u_HairTotalParticles * 0] : 0.0f;
    PositionBuffer[lid].y = valid ? ParticleStateBuffer[offsetPosition + u_HairTotalParticles * 1] : 0.0f;
    PositionBuffer[lid].z = valid ? ParticleStateBuffer[offsetPosition + u_HairTotalParticles * 2] : 0.0f;

    VelocityBuffer[lid].x = valid ? ParticleStateBuffer[offsetVelocity + u_HairTotalParticles * 0] : 0.0f;
    VelocityBuffer[lid].y = valid ? ParticleStateBuffer[offsetVelocity + u_HairTotalParticles * 1] : 0.0f;
    VelocityBuffer[lid].z = valid ? ParticleStateBuffer[offsetVelocity + u_HairTotalParticles * 2] : 0.0f;

    // Remember original particle position.
    vec3 originalPosition = PositionBuffer[lid];

    // Compute position update.
    // TODO Support moving root.
    PositionBuffer[lid] = root ? originalPosition : (originalPosition + VelocityBuffer[lid] * u_DeltaTime + u_ExternalForces * u_DeltaTimeSq);

    // Position before constraints of next particle.
    vec3 preConstraintPosition = PositionBuffer[lid];

    // Apply FTL alternately on even and odd particles.
    for (uint i = 0; i < u_FtlIterations; ++i)
    {
        barrier();

        if (lid % 2 == 0)
        {
            PositionBuffer[lid] = root ? PositionBuffer[lid]
                : (PositionBuffer[plid] + normalize(PositionBuffer[lid] - PositionBuffer[plid]) * u_HairParticleSeparation);
        }

        barrier();

        if (lid % 2 == 1)
        {
            PositionBuffer[lid] = root ? PositionBuffer[lid]
                : (PositionBuffer[plid] + normalize(PositionBuffer[lid] - PositionBuffer[plid]) * u_HairParticleSeparation);
        }
    }

    // Find correction vector.
    vec3 correction = correct ? (PositionBuffer[lid] - preConstraintPosition) : vec3(0);

    // Calculate base velocity.
    VelocityBuffer[lid] = (PositionBuffer[lid] - originalPosition) * u_DeltaTimeInv;

    // Apply correction if required.
    VelocityBuffer[lid] += correction * u_DampingFactor;

    // Write results back to global memory.
    if (valid)
    {
        ParticleStateBuffer[offsetPosition + u_HairTotalParticles * 0] = PositionBuffer[lid].x;
        ParticleStateBuffer[offsetPosition + u_HairTotalParticles * 1] = PositionBuffer[lid].y;
        ParticleStateBuffer[offsetPosition + u_HairTotalParticles * 2] = PositionBuffer[lid].z;

        ParticleStateBuffer[offsetVelocity + u_HairTotalParticles * 0] = VelocityBuffer[lid].x;
        ParticleStateBuffer[offsetVelocity + u_HairTotalParticles * 1] = VelocityBuffer[lid].y;
        ParticleStateBuffer[offsetVelocity + u_HairTotalParticles * 2] = VelocityBuffer[lid].z;
    }
}
