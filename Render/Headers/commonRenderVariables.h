#pragma once
#include <vector>
#include "ecsCommon.h"

static const glm::vec3 UP_VECTOR(0.f, 1.f, 0.f);
static const glm::vec4 UP_VECTOR4(0.f, 1.f, 0.f, 0.f);
extern glm::vec3 COMMON_AMBIENT;
extern glm::vec2 DEPTH_BIAS_PARAMS;

extern ECS::ENTITY_TYPE gameCamera;
extern std::vector<ECS::ENTITY_TYPE> pointLights;
extern ECS::ENTITY_TYPE directionalLight;

//todo: make it right
struct SSAO_VARIABLES {
    bool  turnOffSSAO = false;
    float radius = 8.f;
    float bias = 0.1f;
};

struct DEBUG_VARIABLES {
    int drawMode = 0;
};

extern SSAO_VARIABLES  gSSAODebugVariables;
extern DEBUG_VARIABLES gDebugVariables;