#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "base.cmf"

layout(push_constant) uniform ColorBlock  {
    mat4 modelMatrix;
} pushConstant;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexUV;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec2 outTexCord;

void main() {
    vec4 worldPos = pushConstant.modelMatrix * vec4(inPosition, 1.0f);
    gl_Position = commonBuffer.mWorldViewProj * worldPos;

    outWorldPos = worldPos.xyz;
    outWorldNormal = normalize(pushConstant.modelMatrix * vec4(inNormal, 0.f)).xyz;
    outTexCord = inTexUV;
}