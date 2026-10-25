#version 460 core

out vec2 TexCoords;

void main()
{
    // Generates a fullscreen triangle from gl_VertexID without any vertex buffers
    TexCoords = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(TexCoords * 2.0 - 1.0, 0.0, 1.0);
}
