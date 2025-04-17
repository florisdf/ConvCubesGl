#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

#define TRANS_KEYFRAMES 170

struct InstanceDataStill {
    float color[4];
    float position[3];
    float time;
};

struct InstanceDataTrans {
    float maxDuration;
    int easing;
    int keyframeCount;
    float startTimes[TRANS_KEYFRAMES];
    int endIdxs[TRANS_KEYFRAMES];
};

// Now define the buffer block
layout(std430, binding = 2) buffer InstanceBufferStill {
    InstanceDataStill instancesStill[];
};

layout(std430, binding = 3) buffer InstanceBufferTrans {
    InstanceDataTrans instancesTrans[];
};

float applyEasing(float t, int easingType) {
    switch (easingType) {
        case 0: return t; // LINEAR
        case 1: return t * t; // IN_QUAD
        case 2: return 1.0 - (1.0 - t) * (1.0 - t); // OUT_QUAD
        case 3: return t < 0.5 ? 2.0 * t * t : 1.0 - 2.0 * (1.0 - t) * (1.0 - t); // IN_OUT_QUAD
        case 4: return t * t * t; // IN_CUBIC
        case 5: return 1.0 - pow(1.0 - t, 3.0); // OUT_CUBIC
        case 6: return t < 0.5 ? 4.0 * t * t * t : 1.0 - pow(1.0 - 2.0 * t, 3.0); // IN_OUT_CUBIC
        case 7: return 0; // Hold
        default: return t; // Default to linear
    }
}

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float currentTime;
uniform bool isStill;
uniform bool isOutline;

// Outs
out vec4 fColor;
out vec3 fNormal;

void main()
{
    int instanceID = gl_InstanceID;

    // Compute properties
    vec3 aOffset = vec3(0, 0, -999999);  // Default values
    vec4 aColor = vec4(0, 0, 0, 0);
    if (isStill) {
        InstanceDataStill stillInstance = instancesStill[instanceID];
        if (currentTime >= stillInstance.time) {
            aOffset = vec3(stillInstance.position[0], stillInstance.position[1], stillInstance.position[2]);
            aColor = vec4(stillInstance.color[0], stillInstance.color[1], stillInstance.color[2], stillInstance.color[3]);
        }
    } else {
        InstanceDataStill startInstance = instancesStill[instanceID];
        InstanceDataTrans transInstance = instancesTrans[instanceID];
        float transMaxDuration = transInstance.maxDuration;
        int easing = transInstance.easing;
        for (int i = 0; i < transInstance.keyframeCount; i++) {
            int endIdx = transInstance.endIdxs[i];
            InstanceDataStill endInstance = instancesStill[endIdx];

            float t0 = transInstance.startTimes[i];
            float t1 = endInstance.time;
            if (t0 <= currentTime && currentTime <= t1) {
                //float duration = min(t1 - t0, transMaxDuration);
                float t = (currentTime - t0) / (t1 - t0);
                t = applyEasing(t, easing);

                vec3 p0 = vec3(startInstance.position[0], startInstance.position[1], startInstance.position[2]);
                vec3 p1 = vec3(endInstance.position[0], endInstance.position[1], endInstance.position[2]);
                aOffset = mix(p0, p1, t);

                vec4 c0 = vec4(startInstance.color[0], startInstance.color[1], startInstance.color[2], startInstance.color[3]);
                vec4 c1 = vec4(endInstance.color[0], endInstance.color[1], endInstance.color[2], endInstance.color[3]);
                aColor = mix(c0, c1, t);
                break;
            }
        }
    }

    float aSphereness = 0.0;
    float aScale = 1.0;

    if (!isOutline) aScale *= 0.8;

    // Transform the vertex
    vec3 cubePos = aPos;
    vec3 spherePos = normalize(vec3(cubePos)) / 2;
    vec3 cubeNormal = aNormal;
    vec3 sphereNormal = normalize(spherePos);
    vec3 pos = mix(cubePos, spherePos, aSphereness);

    vec3 normal = normalize(mix(cubeNormal, sphereNormal, aSphereness));

    gl_Position = projection * view * model * vec4((aScale * pos) + aOffset, 1.0);
    fColor = aColor;
    fNormal = normal;
}
