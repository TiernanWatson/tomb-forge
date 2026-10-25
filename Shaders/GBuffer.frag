#version 460 core

layout (location = 0) out vec3 GPosition;
layout (location = 1) out vec3 GNormal;

in vec3 ViewPos;
in vec3 ViewNormal;

void main()
{
    GPosition = ViewPos;
    GNormal = normalize(ViewNormal);
}
