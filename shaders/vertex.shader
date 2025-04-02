#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

#define MAX_KEYFRAMES 4

struct FloatKeyframe {
    float time;
    float value;
    int easing;
};

struct Vec3Keyframe {
    float time;
    vec3 value;
    int easing;
};

struct Vec4Keyframe {
    float time;
    vec4 value;
    int easing;
};

struct InstanceData {
    Vec3Keyframe positionKeyframes[MAX_KEYFRAMES];
    int numPositionKeyframes;

    Vec4Keyframe colorKeyframes[MAX_KEYFRAMES];
    int numColorKeyframes;

    FloatKeyframe spherenessKeyframes[MAX_KEYFRAMES];
    int numSpherenessKeyframes;

    FloatKeyframe scaleKeyframes[MAX_KEYFRAMES];
    int numScaleKeyframes;
};

layout(std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float currentTime;

// Outs
out vec4 fColor;
out vec3 fNormal;


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

float interpolateFloat(FloatKeyframe keyframes[MAX_KEYFRAMES], int keyframeCount, float time) {
    if (keyframeCount == 0) return 1.0; // Default to fully visible

    float value = keyframes[0].value;

    for (int i = 0; i < keyframeCount - 1; i++) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            t = applyEasing(t, keyframes[i].easing);
            value = mix(keyframes[i].value, keyframes[i + 1].value, t);
            break;
        }
    }

    return value;
}

vec3 interpolateVec3(Vec3Keyframe keyframes[MAX_KEYFRAMES], int keyframeCount, float time) {
    if (keyframeCount == 0) return vec3(0.0);

    vec3 value = keyframes[0].value;

    for (int i = 0; i < keyframeCount - 1; i++) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            t = applyEasing(t, keyframes[i].easing);
            value = mix(keyframes[i].value, keyframes[i + 1].value, t);
            break;
        }
    }

    return value;
}

vec4 interpolateVec4(Vec4Keyframe keyframes[MAX_KEYFRAMES], int keyframeCount, float time) {
    if (keyframeCount == 0) return vec4(1.0);

    vec4 value = keyframes[0].value;

    for (int i = 0; i < keyframeCount - 1; i++) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            t = applyEasing(t, keyframes[i].easing);
            value = mix(keyframes[i].value, keyframes[i + 1].value, t);
            break;
        }
    }

    return value;
}

void main()
{
    int instanceID = gl_InstanceID;
    InstanceData instance = instances[instanceID];

    // Compute interpolated properties
    vec3 aOffset = interpolateVec3(instance.positionKeyframes, instance.numPositionKeyframes, currentTime);
    vec4 aColor = interpolateVec4(instance.colorKeyframes, instance.numColorKeyframes, currentTime);
    float aSphereness = interpolateFloat(instance.spherenessKeyframes, instance.numSpherenessKeyframes, currentTime);
    float aScale = interpolateFloat(instance.scaleKeyframes, instance.numScaleKeyframes, currentTime);

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
