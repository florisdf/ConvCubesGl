#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aOffset;
layout (location = 3) in vec3 aColor;

out vec3 fColor;
out vec3 fNormal;

void main()
{
    gl_Position = vec4(aPos + vec3(aOffset, 0.0), 1.0);
    fColor = aColor;
    fNormal = aNormal;
}
