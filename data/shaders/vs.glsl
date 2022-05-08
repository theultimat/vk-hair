#version 450

void main()
{
    const vec2 positions[] = vec2[3]
    (
        vec2(1.0f, 1.0f),
        vec2(-1.0f, 1.0f),
        vec2(0.0f, -1.0f)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0, 1);
}
