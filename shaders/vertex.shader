#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aOffset;
layout (location = 3) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fColor;
out vec3 fNormal;

void main()
{
    gl_Position = projection * view * model * vec4(aPos + aOffset, 1.0);
    fColor = aColor;
    fNormal = aNormal;
}
