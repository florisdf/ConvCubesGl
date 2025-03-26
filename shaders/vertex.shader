#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aOffset;
layout (location = 3) in vec4 aColor;
layout (location = 4) in float aSphereness;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 fColor;
out vec3 fNormal;

void main()
{
    vec4 cubePos = vec4(aPos, 1.0);
    vec4 spherePos = vec4(normalize(vec3(cubePos)) / 2, 1.0);
    vec3 cubeNormal = aNormal;
    vec3 sphereNormal = normalize(vec3(spherePos));
    vec4 pos = mix(cubePos, spherePos, aSphereness);

    vec3 normal = normalize(mix(cubeNormal, sphereNormal, aSphereness));

    gl_Position = projection * view * model * (pos + vec4(aOffset, 1.0));
    fColor = aColor;
    fNormal = normal;
}
