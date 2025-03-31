#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

#version 450 core

layout (location = 0) in vec3 aPos;  // Local vertex position

#define MAX_KEYFRAMES 4

layout(std430, binding = 0) buffer InstanceBuffer {
    struct FloatKeyframe {
        float time;
        float value;
    };

    struct Vec3Keyframe {
        float time;
        vec3 value;
    };

    struct Vec4Keyframe {
        float time;
        vec4 value;
    };

    #version 450 core

layout (location = 0) in vec3 aPos;  // Local vertex position

#define MAX_KEYFRAMES 2

layout(std430, binding = 0) buffer InstanceBuffer {
    struct FloatKeyframe {
        float time;
        float value;
    };

    struct Vec3Keyframe {
        float time;
        vec3 value;
    };

    struct Vec4Keyframe {
        float time;
        vec4 value;
    };

    Vec3Keyframe positionKeyframes[MAX_KEYFRAMES];
    int numPositionKeyframes;

    Vec4Keyframe colorKeyframes[MAX_KEYFRAMES];
    int numColorKeyframes;

    FloatKeyframe spherenessKeyframes[MAX_KEYFRAMES];
    int numSpherenessKeyframes;

    FloatKeyframe scaleKeyframes[MAX_KEYFRAMES];
    int numScaleKeyframes;
};

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float currentTime;

// Outs
out vec4 fColor;
out vec3 fNormal;

float interpolateFloat(FloatKeyframe keyframes[MAX_KEYFRAMES], int keyframeCount, float time) {
    if (keyframeCount == 0) return 1.0; // Default to fully visible

    float value = keyframes[0].value;

    for (int i = 0; i < keyframeCount - 1; i++) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            t = t * t * (3.0 - 2.0 * t); // Smoothstep easing
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
            t = t * t * (3.0 - 2.0 * t);
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
            t = t * t * (3.0 - 2.0 * t);
            value = mix(keyframes[i].value, keyframes[i + 1].value, t);
            break;
        }
    }

    return value;
}

void main()
{
    int instanceID = gl_InstanceID;


    positionKeyframes[MAX_KEYFRAMES];
    colorKeyframes[MAX_KEYFRAMES];
    spherenessKeyframes[MAX_KEYFRAMES];
    scaleKeyframes[MAX_KEYFRAMES];

    // Compute interpolated properties
    vec3 aOffset = interpolateVec3(positionKeyframes[instanceID], numPositionKeyframes[instanceID], currentTime);
    vec3 aColor = interpolateVec4(colorKeyframes[instanceID], numColorKeyframes[instanceID], currentTime);
    vec3 aSphereness = interpolateFloat(spherenessKeyframes[instanceID], numSpherenessKeyframes[instanceID], currentTime);
    vec3 aScale = interpolateFloat(scaleKeyframes[instanceID], numScaleKeyframes[instanceID], currentTime);

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
