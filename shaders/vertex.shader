#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

#define MAX_KEYFRAMES 4

struct Float4Keyframe {
    float value[4];
    float time;
    int easing;
};

struct Float3Keyframe {
    float value[3];
    float time;
    int easing;
};

struct InstanceData {
    Float4Keyframe colorKfs[MAX_KEYFRAMES];
    int numColorKfs;
    Float3Keyframe positionKfs[MAX_KEYFRAMES];
    int numPositionKfs;
};

// Now define the buffer block
layout(std430, binding = 2) buffer InstanceBuffer {
    InstanceData instances[];
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
        default: return t; // Default to linear
    }
}

vec4 interpolateFloat4(Float4Keyframe keyframes[MAX_KEYFRAMES], int keyframeCount, float time) {
    if (keyframeCount == 0) return vec4(1.0);

    float v0f4[4] = keyframes[0].value;
    vec4 value = {v0f4[0], v0f4[1], v0f4[2], v0f4[3]};

    for (int i = 0; i < keyframeCount - 1; i++) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            t = applyEasing(t, keyframes[i].easing);
            float v1f4[4] = keyframes[1].value;
            vec4 v2 = {v1f4[0], v1f4[1], v1f4[2], v1f4[3]};
            float v2f4[4] = keyframes[2].value;
            vec4 v1 = {v2f4[0], v2f4[1], v2f4[2], v2f4[3]};
            value = mix(v1, v2, t);
            break;
        }
    }

    return value;
}


vec3 interpolateFloat3(Float3Keyframe keyframes[MAX_KEYFRAMES], int keyframeCount, float time) {
    if (keyframeCount == 0) return vec3(1.0);

    float v0f3[3] = keyframes[0].value;
    vec3 value = {v0f3[0], v0f3[1], v0f3[2]};

    for (int i = 0; i < keyframeCount - 1; i++) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            t = applyEasing(t, keyframes[i].easing);
            float v1f3[3] = keyframes[1].value;
            vec3 v2 = {v1f3[0], v1f3[1], v1f3[2]};
            float v2f3[3] = keyframes[2].value;
            vec3 v1 = {v2f3[0], v2f3[1], v2f3[2]};
            value = mix(v1, v2, t);
            break;
        }
    }

    return value;
}

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float currentTime;

// Outs
out vec4 fColor;
out vec3 fNormal;

vec4 getColor(int index) { 
    return interpolateFloat4(instances[index].colorKfs, instances[index].numColorKfs, currentTime);
}

vec3 getPosition(int index) {
    return interpolateFloat3(instances[index].positionKfs, instances[index].numPositionKfs, currentTime);
}

void main()
{
    int instanceID = gl_InstanceID;
    InstanceData instance = instances[instanceID];

    // Compute interpolated properties
    vec3 aOffset = getPosition(instanceID);//vec3(instanceID%224-112, instanceID/224-112, 0.0);//interpolateVec3(instance.positionKeyframes, instance.numPositionKeyframes, currentTime);
    vec4 aColor = getColor(instanceID);//instance.colorKeyframes[0].value;//interpolateVec4(instance.colorKeyframes, instance.numColorKeyframes, currentTime);
    float aSphereness = 0.0;//interpolateFloat(instance.spherenessKeyframes, instance.numSpherenessKeyframes, currentTime);
    float aScale = 1.0;//interpolateFloat(instance.scaleKeyframes, instance.numScaleKeyframes, currentTime);

    // Transform the vertex
    vec4 cubePos = vec4(aPos, 1.0);
    vec4 spherePos = vec4(normalize(vec3(cubePos)) / 2, 1.0);
    vec3 cubeNormal = aNormal;
    vec3 sphereNormal = normalize(vec3(spherePos));
    vec4 pos = mix(cubePos, spherePos, aSphereness);

    vec3 normal = normalize(mix(cubeNormal, sphereNormal, aSphereness));

    gl_Position = projection * view * model * (aScale * pos + vec4(aOffset, 1.0));
    fColor = aColor;
    fNormal = normal;
}
